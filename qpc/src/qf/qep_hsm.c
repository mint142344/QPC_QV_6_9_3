/**
 * @file
 * @brief ::QHsm implementation
 * @ingroup qep
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.2
 * Last updated on  2020-12-16
 *
 *                    Q u a n t u m  L e a P s
 *                    ------------------------
 *                    Modern Embedded Software
 *
 * Copyright (C) 2005-2020 Quantum Leaps, LLC. All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Alternatively, this program may be distributed and modified under the
 * terms of Quantum Leaps commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for
 * licensees interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <www.gnu.org/licenses>.
 *
 * Contact information:
 * <www.state-machine.com/licensing>
 * <info@state-machine.com>
 ******************************************************************************
 * @endcond
 */
#define QP_IMPL       /* this is QP implementation */
#include "qep_port.h" /* QEP port */
#include "qassert.h"  /* QP embedded systems-friendly assertions */
#ifdef Q_SPY          /* QS software tracing enabled? */
#include "qs_port.h"  /* QS port */
#include "qs_pkg.h"   /* QS facilities for pre-defined trace records */
#else
#include "qs_dummy.h" /* disable the QS software tracing */
#endif                /* Q_SPY */

Q_DEFINE_THIS_MODULE("qep_hsm")

/****************************************************************************/
char_t const QP_versionStr[7] = QP_VERSION_STR;

/****************************************************************************/
/*! 内部 QEP 常量 */
enum {
    QEP_EMPTY_SIG_ = 0, /*!< 仅供内部使用的保留空信号 */

    /*! HSM 中状态嵌套的最大深度(包含顶层), 必须 >= 3 */
    QHSM_MAX_NEST_DEPTH_ = 6
};

/**
 * @brief 保留事件
 * 静态预分配的标准事件, QEP 事件处理器发送这些事件给 QHsm 风格的状态机的状态处理函数,
 * 用于执行入口动作, 退出动作和初始转换.
 */
static QEvt const QEP_reservedEvt_[] = {
    {(QSignal)QEP_EMPTY_SIG_, 0U, 0U},
    {(QSignal)Q_ENTRY_SIG, 0U, 0U},
    {(QSignal)Q_EXIT_SIG, 0U, 0U},
    {(QSignal)Q_INIT_SIG, 0U, 0U}};

/** 在一个状态转换函数中执行 保留事件动作
 *	QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_)
 *	QEP_TRIG_(t, Q_INIT_SIG)
 */
#define QEP_TRIG_(state_, sig_) \
    ((*(state_))(me, &QEP_reservedEvt_[(sig_)]))

// 状态处理函数执行退出动作Q_EXIT_SIG
#define QEP_EXIT_(state_, qs_id_)                                       \
    do {                                                                \
        if (QEP_TRIG_((state_), Q_EXIT_SIG) == (QState)Q_RET_HANDLED) { \
            QS_BEGIN_PRE_(QS_QEP_STATE_EXIT, (qs_id_))                  \
            QS_OBJ_PRE_(me);                                            \
            QS_FUN_PRE_(state_);                                        \
            QS_END_PRE_()                                               \
        }                                                               \
    } while (false)

// 状态处理函数执行进入动作Q_ENTRY_SIG
#define QEP_ENTER_(state_, qs_id_)                                       \
    do {                                                                 \
        if (QEP_TRIG_((state_), Q_ENTRY_SIG) == (QState)Q_RET_HANDLED) { \
            QS_BEGIN_PRE_(QS_QEP_STATE_ENTRY, (qs_id_))                  \
            QS_OBJ_PRE_(me);                                             \
            QS_FUN_PRE_(state_);                                         \
            QS_END_PRE_()                                                \
        }                                                                \
    } while (false)

/*! 辅助函数, 用于执行状态转换 */
#ifdef Q_SPY
static int_fast8_t QHsm_tran_(QHsm *const me,
                              QStateHandler path[QHSM_MAX_NEST_DEPTH_],
                              uint_fast8_t const qs_id);
#else
static int_fast8_t QHsm_tran_(QHsm *const me,
                              QStateHandler path[QHSM_MAX_NEST_DEPTH_]);
#endif

/****************************************************************************/
/**
 * @brief
 * 执行 HSM 初始化的第一步, 将初始伪状态分配给状态机当前的活动状态.
 *
 * @param[in,out] me      指针
 * @param[in]     initial 指向派生状态机中最顶层初始状态处理函数的指针
 *
 * @note 仅能由派生状态机的构造函数调用.
 *
 * @note 必须在 QHSM_INIT() 之前 \b 且仅调用一次
 */
void QHsm_ctor(QHsm *const me, QStateHandler initial)
{
    static struct QHsmVtable const vtable = {/* QHsm virtual table */
                                             &QHsm_init_,
                                             &QHsm_dispatch_
#ifdef Q_SPY
                                             ,
                                             &QHsm_getStateHandler_
#endif
    };
    me->vptr      = &vtable;
    me->state.fun = Q_STATE_CAST(&QHsm_top);
    me->temp.fun  = initial;
}

/****************************************************************************/
/**
 * @brief
 * 执行 HSM 中最顶层的初始转换.
 *
 * @param[in,out] me   指针
 * @param[in]     e    额外参数指针(可以为 NULL)
 * @param[in]     qs_id 状态机的 QS-id(用于 QS 本地过滤)
 *
 * @note 必须在 QHsm_ctor() 之后 \b 且仅调用一次
 */
#ifdef Q_SPY
void QHsm_init_(QHsm *const me, void const *const e,
                uint_fast8_t const qs_id)
#else
void QHsm_init_(QHsm *const me, void const *const e)
#endif
{
    QStateHandler t = me->state.fun;
    QState r;
    QS_CRIT_STAT_

    /** @pre 虚函数指针必须已初始化, 最顶层初始转换必须已初始化, 且初始转换尚未执行 */
    Q_REQUIRE_ID(200, (me->vptr != (struct QHsmVtable *)0) && (me->temp.fun != Q_STATE_CAST(0)) && (t == Q_STATE_CAST(&QHsm_top)));

    /* 执行最顶层初始转换 */
    r = (*me->temp.fun)(me, Q_EVT_CAST(QEvt));

    /* 最顶层初始转换必须被执行 */
    Q_ASSERT_ID(210, r == (QState)Q_RET_TRAN);

    QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qs_id)
    QS_OBJ_PRE_(me);           /* 当前状态机对象 */
    QS_FUN_PRE_(t);            /* 源状态 */
    QS_FUN_PRE_(me->temp.fun); /* 初始转换的目标状态 */
    QS_END_PRE_()

    /* 沿状态层次结构递归执行初始转换... */
    do {
        QStateHandler path[QHSM_MAX_NEST_DEPTH_]; /* 转换入口路径数组 */
        int_fast8_t ip = 0;                       /* 转换路径索引 */

        path[0] = me->temp.fun;
        (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
        while (me->temp.fun != t) {
            ++ip;
            Q_ASSERT_ID(220, ip < (int_fast8_t)Q_DIM(path));
            path[ip] = me->temp.fun;
            (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
        }
        me->temp.fun = path[0];

        /* 按逆序回溯进入路径(期望顺序)... */
        do {
            QEP_ENTER_(path[ip], qs_id); /* 进入 path[ip] */
            --ip;
        } while (ip >= 0);

        t = path[0]; /* 当前状态成为新的源状态 */

        r = QEP_TRIG_(t, Q_INIT_SIG); /* 执行初始转换 */

#ifdef Q_SPY
        if (r == (QState)Q_RET_TRAN) {
            QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qs_id)
            QS_OBJ_PRE_(me);           /* 当前状态机对象 */
            QS_FUN_PRE_(t);            /* 源状态 */
            QS_FUN_PRE_(me->temp.fun); /* 初始转换的目标状态 */
            QS_END_PRE_()
        }
#endif /* Q_SPY */

    } while (r == (QState)Q_RET_TRAN);

    QS_BEGIN_PRE_(QS_QEP_INIT_TRAN, qs_id)
    QS_TIME_PRE_();  /* 时间戳 */
    QS_OBJ_PRE_(me); /* 当前状态机对象 */
    QS_FUN_PRE_(t);  /* 新的活动状态 */
    QS_END_PRE_()

    me->state.fun = t; /* 改变当前活动状态 */
    me->temp.fun  = t; /* 将配置标记为稳定 */
}

/****************************************************************************/
/**
 * @brief
 * QHsm_top() 是所有从 ::QHsm 派生的 HSM 中状态层次的最终根状态
 *
 * @param[in] me 指针
 * @param[in] e  指向要分发到状态机的事件的指针
 *
 * @returns 总是返回 #Q_RET_IGNORED, 表示顶层状态会忽略所有事件.
 *
 * @note 该状态处理函数的参数未使用, 仅为了符合状态处理函数签名 ::QStateHandler.
 */
QState QHsm_top(void const *const me, QEvt const *const e)
{
    (void)me;                     /* suppress the "unused parameter" compiler warning */
    (void)e;                      /* suppress the "unused parameter" compiler warning */
    return (QState)Q_RET_IGNORED; /* the top state ignores all events */
}

/****************************************************************************/
/**
 * @brief
 * 将事件分发给层次状态机 (HSM) 处理.
 * 处理一个事件相当于执行一次"运行到完成"(RTC)步骤.
 *
 * @param[in,out] me   指针
 * @param[in]     e    指向要分发到 HSM 的事件的指针
 * @param[in]     qs_id 状态机的 QS-id(用于 QS 本地过滤)
 *
 * @note
 * 该函数应仅通过虚函数表调用 (参见 QHSM_DISPATCH()) 不应在应用程序中直接调用
 */
#ifdef Q_SPY
void QHsm_dispatch_(QHsm *const me, QEvt const *const e,
                    uint_fast8_t const qs_id)
#else
void QHsm_dispatch_(QHsm *const me, QEvt const *const e)
#endif
{
    QStateHandler t = me->state.fun;
    QStateHandler s;
    QState r;
    QS_CRIT_STAT_

    /** @pre 当前状态必须已初始化, 且状态配置必须稳定 */
    Q_REQUIRE_ID(400, (t != Q_STATE_CAST(0)) && (t == me->temp.fun));

    QS_BEGIN_PRE_(QS_QEP_DISPATCH, qs_id)
    QS_TIME_PRE_();      /* 时间戳 */
    QS_SIG_PRE_(e->sig); /* 事件的信号 */
    QS_OBJ_PRE_(me);     /* 当前状态机对象 */
    QS_FUN_PRE_(t);      /* 当前状态 */
    QS_END_PRE_()

    /* 分层处理事件... */
    do {
        s = me->temp.fun;
        r = (*s)(me, e); /* 调用状态处理函数 s */

        if (r == (QState)Q_RET_UNHANDLED) { /* 因守卫条件未处理? */

            QS_BEGIN_PRE_(QS_QEP_UNHANDLED, qs_id)
            QS_SIG_PRE_(e->sig); /* 事件的信号 */
            QS_OBJ_PRE_(me);     /* 当前状态机对象 */
            QS_FUN_PRE_(s);      /* 当前状态 */
            QS_END_PRE_()

            r = QEP_TRIG_(s, QEP_EMPTY_SIG_); /* 查找 s 的父状态 */
        }
    } while (r == (QState)Q_RET_SUPER);

    /* 是否发生状态转换? */
    if (r >= (QState)Q_RET_TRAN) {
        QStateHandler path[QHSM_MAX_NEST_DEPTH_];
        int_fast8_t ip;

        path[0] = me->temp.fun; /* 保存转换目标 */
        path[1] = t;
        path[2] = s;

        /* 从当前状态退出直到转换源 s... */
        for (; t != s; t = me->temp.fun) {
            if (QEP_TRIG_(t, Q_EXIT_SIG) == (QState)Q_RET_HANDLED) {
                QS_BEGIN_PRE_(QS_QEP_STATE_EXIT, qs_id)
                QS_OBJ_PRE_(me); /* 当前状态机对象 */
                QS_FUN_PRE_(t);  /* 已退出的状态 */
                QS_END_PRE_()

                (void)QEP_TRIG_(t, QEP_EMPTY_SIG_); /* 查找 t 的父状态 */
            }
        }

#ifdef Q_SPY
        ip = QHsm_tran_(me, path, qs_id);
#else
        ip = QHsm_tran_(me, path);
#endif

#ifdef Q_SPY
        if (r == (QState)Q_RET_TRAN_HIST) {

            QS_BEGIN_PRE_(QS_QEP_TRAN_HIST, qs_id)
            QS_OBJ_PRE_(me);      /* 当前状态机对象 */
            QS_FUN_PRE_(t);       /* 转换源状态 */
            QS_FUN_PRE_(path[0]); /* 历史转换目标 */
            QS_END_PRE_()
        }
#endif /* Q_SPY */

        /* 按逆序回溯进入路径(期望顺序)... */
        for (; ip >= 0; --ip) {
            QEP_ENTER_(path[ip], qs_id); /* 进入 path[ip] */
        }

        t            = path[0]; /* 将目标状态保存到寄存器 */
        me->temp.fun = t;       /* 更新下一个状态 */

        /* 深入目标层次结构... */
        while (QEP_TRIG_(t, Q_INIT_SIG) == (QState)Q_RET_TRAN) {

            QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qs_id)
            QS_OBJ_PRE_(me);           /* 当前状态机对象 */
            QS_FUN_PRE_(t);            /* 源(伪)状态 */
            QS_FUN_PRE_(me->temp.fun); /* 转换目标 */
            QS_END_PRE_()

            ip      = 0;
            path[0] = me->temp.fun;

            (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_); /* 查找父状态 */

            while (me->temp.fun != t) {
                ++ip;
                path[ip] = me->temp.fun;
                (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_); /* 查找父状态 */
            }
            me->temp.fun = path[0];

            /* 进入路径不能溢出 */
            Q_ASSERT_ID(410, ip < QHSM_MAX_NEST_DEPTH_);

            /* 按逆序回溯进入路径(正确顺序)... */
            do {
                QEP_ENTER_(path[ip], qs_id); /* 进入 path[ip] */
                --ip;
            } while (ip >= 0);

            t = path[0]; /* 当前状态成为新的源状态 */
        }

        QS_BEGIN_PRE_(QS_QEP_TRAN, qs_id)
        QS_TIME_PRE_();      /* 时间戳 */
        QS_SIG_PRE_(e->sig); /* 事件信号 */
        QS_OBJ_PRE_(me);     /* 当前状态机对象 */
        QS_FUN_PRE_(s);      /* 转换源状态 */
        QS_FUN_PRE_(t);      /* 新的活动状态 */
        QS_END_PRE_()
    }

#ifdef Q_SPY
    else if (r == (QState)Q_RET_HANDLED) {

        QS_BEGIN_PRE_(QS_QEP_INTERN_TRAN, qs_id)
        QS_TIME_PRE_();      /* 时间戳 */
        QS_SIG_PRE_(e->sig); /* 事件信号 */
        QS_OBJ_PRE_(me);     /* 当前状态机对象 */
        QS_FUN_PRE_(s);      /* 源状态 */
        QS_END_PRE_()

    } else {

        QS_BEGIN_PRE_(QS_QEP_IGNORED, qs_id)
        QS_TIME_PRE_();             /* 时间戳 */
        QS_SIG_PRE_(e->sig);        /* 事件信号 */
        QS_OBJ_PRE_(me);            /* 当前状态机对象 */
        QS_FUN_PRE_(me->state.fun); /* 当前状态 */
        QS_END_PRE_()
    }
#endif /* Q_SPY */

    me->state.fun = t; /* 改变当前活动状态 */
    me->temp.fun  = t; /* 标记配置为稳定 */
}

/****************************************************************************/
/**
 * @brief
 * 静态辅助函数, 用于在层次状态机 (HSM) 中执行状态转换序列.
 *
 * @param[in,out] me   指针
 * @param[in,out] path 指向状态处理函数数组的指针, 用于执行进入动作
 * @param[in]     qs_id 状态机的 QS-id(用于 QS 本地过滤)
 *
 * @returns
 * 存储在 @p path 参数中的进入路径深度
 */
#ifdef Q_SPY
static int_fast8_t QHsm_tran_(QHsm *const me,
                              QStateHandler path[QHSM_MAX_NEST_DEPTH_],
                              uint_fast8_t const qs_id)
#else
static int_fast8_t QHsm_tran_(QHsm *const me,
                              QStateHandler path[QHSM_MAX_NEST_DEPTH_])
#endif
{
    int_fast8_t ip = -1; /* 状态转换进入路径索引 */
    int_fast8_t iq;      /* 辅助状态转换进入路径索引 */
    QStateHandler t       = path[0];
    QStateHandler const s = path[2];
    QState r;
    QS_CRIT_STAT_

    /* (a) 检查源状态是否等于目标状态(自转换)... */
    if (s == t) {
        QEP_EXIT_(s, qs_id); /* 退出源状态 */
        ip = 0;              /* 进入目标状态 */
    } else {
        (void)QEP_TRIG_(t, QEP_EMPTY_SIG_); /* 查找目标状态的父状态 */

        t = me->temp.fun;

        /* (b) 检查源状态是否等于目标状态的父状态... */
        if (s == t) {
            ip = 0; /* 进入目标状态 */
        } else {
            (void)QEP_TRIG_(s, QEP_EMPTY_SIG_); /* 查找源状态的父状态 */

            /* (c) 检查源状态的父状态是否等于目标状态的父状态... */
            if (me->temp.fun == t) {
                QEP_EXIT_(s, qs_id); /* 退出源状态 */
                ip = 0;              /* 进入目标状态 */
            } else {
                /* (d) 检查源状态的父状态是否等于目标状态... */
                if (me->temp.fun == path[0]) {
                    QEP_EXIT_(s, qs_id); /* 退出源状态 */
                } else {
                    /* (e) 检查源状态的剩余父状态是否等于目标状态的父父状态... 并沿途存储进入路径 */
                    iq      = 0;            /* 标记 LCA(最近公共祖先)未找到 */
                    ip      = 1;            /* 进入目标状态及其父状态 */
                    path[1] = t;            /* 保存目标状态的父状态 */
                    t       = me->temp.fun; /* 保存源状态的父状态 */

                    /* 查找目标状态的父父状态... */
                    r = QEP_TRIG_(path[1], QEP_EMPTY_SIG_);
                    while (r == (QState)Q_RET_SUPER) {
                        ++ip;
                        path[ip] = me->temp.fun; /* 存储进入路径 */
                        if (me->temp.fun == s) { /* 是否为源状态? */
                            iq = 1;              /* 标记找到 LCA */

                            /* 进入路径不能溢出 */
                            Q_ASSERT_ID(510,
                                        ip < QHSM_MAX_NEST_DEPTH_);
                            --ip;                      /* 不进入源状态 */
                            r = (QState)Q_RET_HANDLED; /* 结束循环 */
                        }
                        /* 不是源状态, 继续向上查找 */
                        else {
                            r = QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
                        }
                    }

                    /* LCA 尚未找到? */
                    if (iq == 0) {

                        /* 进入路径不能溢出 */
                        Q_ASSERT_ID(520, ip < QHSM_MAX_NEST_DEPTH_);

                        QEP_EXIT_(s, qs_id); /* 退出源状态 */

                        /* (f) 检查源状态的其余父状态是否等于目标状态的父父状态... */
                        iq = ip;
                        r  = (QState)Q_RET_IGNORED; /* LCA 未找到 */
                        do {
                            if (t == path[iq]) {            /* 是否为 LCA？ */
                                r  = (QState)Q_RET_HANDLED; /* 找到 LCA */
                                ip = iq - 1;                /* 不进入 LCA */
                                iq = -1;                    /* 结束循环 */
                            } else {
                                --iq; /* 尝试目标状态的更低父状态 */
                            }
                        } while (iq >= 0);

                        /* 仍未找到 LCA? */
                        if (r != (QState)Q_RET_HANDLED) {
                            /* (g) 检查每个源状态的父状态与每个目标状态的父状态... */
                            r = (QState)Q_RET_IGNORED; /* 保持循环 */
                            do {
                                /* 退出 t 状态? */
                                if (QEP_TRIG_(t, Q_EXIT_SIG) == (QState)Q_RET_HANDLED) {
                                    QS_BEGIN_PRE_(QS_QEP_STATE_EXIT, qs_id)
                                    QS_OBJ_PRE_(me);
                                    QS_FUN_PRE_(t);
                                    QS_END_PRE_()

                                    (void)QEP_TRIG_(t, QEP_EMPTY_SIG_);
                                }
                                t  = me->temp.fun; /* 设置为 t 的父状态 */
                                iq = ip;
                                do {
                                    /* 是否为 LCA? */
                                    if (t == path[iq]) {
                                        /* 不进入 LCA */
                                        ip = (int_fast8_t)(iq - 1);
                                        iq = -1; /* 跳出内层循环 */
                                        /* 跳出外层循环 */
                                        r = (QState)Q_RET_HANDLED;
                                    } else {
                                        --iq;
                                    }
                                } while (iq >= 0);
                            } while (r != (QState)Q_RET_HANDLED);
                        }
                    }
                }
            }
        }
    }
    return ip;
}

/****************************************************************************/
#ifdef Q_SPY
QStateHandler QHsm_getStateHandler_(QHsm *const me)
{
    return me->state.fun;
}
#endif

/****************************************************************************/
/**
 * @brief
 * 测试一个派生自 QHsm 的状态机是否处于给定状态.
 *
 * @note 对于 HSM 来说, "处于某状态" 还意味着可能处于该状态的父状态中.
 *
 * @param[in] me    指针
 * @param[in] state 指向要测试的状态处理函数
 *
 * @returns
 * 如果 HSM "处于" @p state, 则返回 true, 否则返回 false
 */
bool QHsm_isIn(QHsm *const me, QStateHandler const state)
{
    bool inState = false; /* 假设 HSM 不在指定状态 */
    QState r;

    /** @pre 状态配置必须是稳定的 */
    Q_REQUIRE_ID(600, me->temp.fun == me->state.fun);

    /* 从底层向上扫描状态层次 */
    do {
        /* do the states match? */
        if (me->temp.fun == state) {
            inState = true;                  /* 匹配成功 */
            r       = (QState)Q_RET_IGNORED; /* 退出循环 */
        } else {
            r = QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
        }
    } while (r != (QState)Q_RET_IGNORED); /* 未到达 QHsm_top() 状态 */
    me->temp.fun = me->state.fun; /* 恢复稳定状态配置 */

    return inState; /* 返回结果 */
}

/****************************************************************************/
/**
 * @brief
 * 查找给定 @c parent 的子状态, 使该子状态是当前活动状态的祖先.
 * 该函数主要用于支持派生自 QHsm 的状态机中的 \b 浅历史 转换.
 *
 * @param[in] me     指针
 * @param[in] parent 指向父状态处理函数
 *
 * @returns
 * 给定 @c parent 的子状态, 该子状态是当前活动状态的祖先.
 * 对于当前活动状态就是给定 @c parent 的情况, 函数返回 @c parent.
 *
 * @note
 * 该函数设计用于状态转换期间调用, 因此不一定从稳定状态配置开始.
 * 但是, 函数在退出时会建立稳定状态配置.
 */
QStateHandler QHsm_childState_(QHsm *const me,
                               QStateHandler const parent)
{
    QStateHandler child = me->state.fun; /* 从当前状态开始 */
    bool isFound        = false;         /* 初始假设未找到子状态 */
    QState r;

    /* 建立稳定状态配置 */
    me->temp.fun = me->state.fun;
    do {
        /* 当前 child 的父状态是否是 parent? */
        if (me->temp.fun == parent) {
            isFound = true;                  /* 找到子状态 */
            r       = (QState)Q_RET_IGNORED; /* 退出循环 */
        } else {
            child = me->temp.fun;
            r     = QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
        }
    } while (r != (QState)Q_RET_IGNORED); /* 未到达 QHsm_top() 状态 */
    me->temp.fun = me->state.fun; /* 建立稳定状态配置 */

    /** @post 必须找到子状态 */
    Q_ENSURE_ID(810, isFound != false);
#ifdef Q_NASSERT
    (void)isFound; /* avoid compiler warning about unused variable */
#endif

    return child; /* 返回子状态 */
}
