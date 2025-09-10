/**
 * @file
 * @brief ::QTimeEvt implementation and QF system clock tick QF_tickX_())
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.1
 * Last updated on  2020-09-03
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
#define QP_IMPL      /* this is QP implementation */
#include "qf_port.h" /* QF port */
#include "qf_pkg.h"  /* QF package-scope interface */
#include "qassert.h" /* QP embedded systems-friendly assertions */
#ifdef Q_SPY         /* QS software tracing enabled? */
#include "qs_port.h" /* QS port */
#include "qs_pkg.h"  /* QS facilities for pre-defined trace records */
#else
#include "qs_dummy.h" /* disable the QS software tracing */
#endif                /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_time")

/* Package-scope objects ****************************************************/
QTimeEvt QF_timeEvtHead_[QF_MAX_TICK_RATE]; /* 时间事件链表头 */

/****************************************************************************/
#ifdef Q_SPY
/**
 * @brief
 * 该函数必须周期性地从系统时钟滴答中断(ISR)或任务中调用,
 * 以便 QF 可以管理分配给给定系统时钟滴答速率的超时事件.
 *
 * @param[in] tickRate 系统时钟滴答速率, 本次调用服务的范围 [1..15]
 * @param[in] sender   指向发送者对象的指针(仅用于 QS 跟踪)
 *
 * @note
 * 该函数应仅通过宏 QF_TICK_X() 调用.
 *
 * @note
 * 使用不同 @p tickRate 参数调用 QF_tickX_() 可以相互抢占.
 * 例如, 高速率的滴答可能由中断服务, 而其他滴答由任务(活动对象)处理.
 */
void QF_tickX_(uint_fast8_t const tickRate, void const *const sender)
#else
void QF_tickX_(uint_fast8_t const tickRate)
#endif
{
    QTimeEvt *prev = &QF_timeEvtHead_[tickRate];
    QF_CRIT_STAT_

    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_TICK, 0U)
    ++prev->ctr;
    QS_TEC_PRE_(prev->ctr); /* 滴答计数器 */
    QS_U8_PRE_(tickRate);   /* 滴答速率 */
    QS_END_NOCRIT_PRE_()

    /* 遍历该速率下的时间事件链表... */
    for (;;) {
        QTimeEvt *t = prev->next; /* 移动到下一个时间事件 */

        /* 到达链表末尾? */
        if (t == (QTimeEvt *)0) {

            /* 自上次 QF_tickX_() 运行后是否有新的时间事件激活? */
            if (QF_timeEvtHead_[tickRate].act != (void *)0) {

                /* 完整性检查 */
                Q_ASSERT_CRIT_(110, prev != (QTimeEvt *)0);
                prev->next                    = (QTimeEvt *)QF_timeEvtHead_[tickRate].act;
                QF_timeEvtHead_[tickRate].act = (void *)0;
                t                             = prev->next; /* 切换到新的列表 */
            } else {
                break; /* * 当前激活的所有时间事件已处理完 */
            }
        }

        /* 时间事件是否被计划移除? */
        if (t->ctr == 0U) {
            prev->next = t->next;
            /* 将时间事件 't' 标记为未链接 */
            t->super.refCtr_ &= (uint8_t)(~TE_IS_LINKED & 0xFFU);
            /* 不要移动 prev 指针 */
            QF_CRIT_X_(); /* 退出临界区以减少延迟 */

            /* 防止临界区合并, 参见下面 NOTE1 */
            QF_CRIT_EXIT_NOP();
        } else {
            --t->ctr;

            /* 时间事件即将到期? */
            if (t->ctr == 0U) {
                QActive *act = (QActive *)t->act; /* volatile 临时变量 */

                /* 周期性时间事件? */
                if (t->interval != 0U) {
                    t->ctr = t->interval; /* 重新激活时间事件 */
                    prev   = t;           /* 移动到该时间事件 */
                } else {
                    /* 一次性时间事件: 自动取消激活disarm */
                    prev->next = t->next;
                    /* 将时间事件 't' 标记为未链接 */
                    t->super.refCtr_ &= (uint8_t)(~TE_IS_LINKED & 0xFFU);
                    /* 不要移动 prev 指针 */

                    QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_AUTO_DISARM, act->prio)
                    QS_OBJ_PRE_(t);       /* this time event object */
                    QS_OBJ_PRE_(act);     /* the target AO */
                    QS_U8_PRE_(tickRate); /* tick rate */
                    QS_END_NOCRIT_PRE_()
                }

                QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_POST, act->prio)
                QS_TIME_PRE_();            /* timestamp */
                QS_OBJ_PRE_(t);            /* the time event object */
                QS_SIG_PRE_(t->super.sig); /* signal of this time event */
                QS_OBJ_PRE_(act);          /* the target AO */
                QS_U8_PRE_(tickRate);      /* tick rate */
                QS_END_NOCRIT_PRE_()

                QF_CRIT_X_(); /* 在投递事件前退出临界区 */

                /* QACTIVE_POST() 内部会在队列溢出时断言 */
                QACTIVE_POST(act, &t->super, sender);
            } else {
                prev = t;     /* 移动到该时间事件 */
                QF_CRIT_X_(); /* 退出临界区以减少延迟 */

                /* 防止临界区合并, 参见 NOTE1 */
                QF_CRIT_EXIT_NOP();
            }
        }
        QF_CRIT_E_(); /* 重新进入临界区继续处理 */
    }
    QF_CRIT_X_();
}

/*****************************************************************************
 * \b NOTE1:
 * 在某些 QF 移植版本中, 退出临界区的操作可能仅在下一条机器指令生效.
 * 如果下一条指令是进入另一个临界区, 那么临界区实际上不会退出,
 * 两个相邻的临界区会被合并.
 *
 * QF_CRIT_EXIT_NOP() 宏包含最少的代码, 用于防止在可能发生合并的 QF 移植版本中出现临界区合并.
 */

/****************************************************************************/
/**
 * @brief
 * 检查在给定的系统时钟滴答速率下 是否有时间事件被激活.
 *
 * @param[in] tickRate  要检查的系统时钟滴答速率.
 *
 * @returns
 * 如果在给定的滴答速率下没有激活的时间事件, 返回 'true';
 * 否则返回 'false'.
 *
 * @note
 * 该函数应在临界区内调用.
 */
bool QF_noTimeEvtsActiveX(uint_fast8_t const tickRate)
{
    bool inactive;

    if (QF_timeEvtHead_[tickRate].next != (QTimeEvt *)0) {
        inactive = false; /* 有下一个时间事件，说明有事件激活 */
    } else if ((QF_timeEvtHead_[tickRate].act != (void *)0)) {
        inactive = false; /* act 指针不为空，也说明有事件激活 */
    } else {
        inactive = true; /* 没有事件激活 */
    }
    return inactive;
}

/****************************************************************************/
/**
 * @brief
 * 创建时间事件时, 必须将其绑定到特定的活动对象 @p act, 滴答速率 @p tickRate
 * 以及事件信号 @p sig, 之后不能更改这些属性。
 *
 * @param[in,out] me       指向时间事件对象的指针
 * @param[in]     act      指向与该时间事件关联的活动对象. 时间事件将投递给该活动对象.
 * @param[in]     sig      与该时间事件关联的信号.
 * @param[in]     tickRate 系统时钟滴答速率, 与该时间事件关联, 范围 [0..15].
 *
 * @note
 * 每个时间事件对象都应仅调用一次构造函数, 在激活时间事件之前
 * 初始化给定 AO 关联的时间事件的理想位置是 AO 的构造函数中.
 */
void QTimeEvt_ctorX(QTimeEvt *const me, QActive *const act,
                    enum_t const sig, uint_fast8_t tickRate)
{
    /** @pre 信号必须有效, 滴答速率在有效范围内 */
    Q_REQUIRE_ID(300, (sig >= (enum_t)Q_USER_SIG) && (tickRate < QF_MAX_TICK_RATE));

    me->next      = (QTimeEvt *)0; /* 下一个时间事件指针置空 */
    me->ctr       = 0U;            /* 计数器置零 */
    me->interval  = 0U;            /* 周期间隔置零 */
    me->super.sig = (QSignal)sig;  /* 记录事件信号 */

    /* 为了向后兼容 QTimeEvt_ctor(), 活动对象指针可以未初始化(NULL),
     * 在前置条件中不做验证. 活动对象指针会在 QTimeEvt_arm_() 和 QTimeEvt_rearm() 中进行验证.
     */
    me->act = act;

    /* 将 POOL_ID 置零只适用于非事件池分配的事件, 对于时间事件必须是这种情况 */
    me->super.poolId_ = 0U;

    /* The refCtr_ attribute is not used in time events, so it is
     * reused to hold the tickRate as well as other information
     */
    /* 时间事件不使用 refCtr_, 因此可重用来保存 tickRate 及其他信息 */
    me->super.refCtr_ = (uint8_t)tickRate;
}

/****************************************************************************/
/**
 * @brief
 * 激活一个时间事件, 使其在指定的时钟滴答数后触发, 并可设置指定的周期间隔.
 * 如果 interval 为零, 则该时间事件为一次性时间事件.
 * 时间事件会直接按照 FIFO 策略投递到所属活动对象的事件队列中.
 *
 * @param[in,out] me       指向时间事件对象的指针
 * @param[in]     nTicks   重新激活时间事件的时钟滴答数(按关联的滴答速率计)
 * @param[in]     interval 周期时间事件的间隔(单位: 时钟滴答数)
 *
 * @note
 * 一次性时间事件在投递后会自动解除激活; 周期时间事件(interval != 0)会自动重新激活.
 *
 * @note
 * 时间事件可以随时通过调用 QTimeEvt_disarm() 解除激活.
 * 也可以通过 QTimeEvt_rearm() 重新激活, 设置不同的滴答数.
 */
void QTimeEvt_armX(QTimeEvt *const me,
                   QTimeEvtCtr const nTicks, QTimeEvtCtr const interval)
{
    uint_fast8_t tickRate = ((uint_fast8_t)me->super.refCtr_ & TE_TICK_RATE); /* 获取滴答速率 */
    QTimeEvtCtr ctr       = me->ctr;                                          /* 临时保存当前计数 */
#ifdef Q_SPY
    uint_fast8_t const qs_id = ((QActive *)(me->act))->prio;
#endif
    QF_CRIT_STAT_

    /** @pre 前置条件: 宿主 AO 必须有效, 时间事件必须未激活,时钟滴答数不能为零, 信号必须有效 */
    Q_REQUIRE_ID(400, (me->act != (void *)0) && (ctr == 0U) && (nTicks != 0U) && (tickRate < (uint_fast8_t)QF_MAX_TICK_RATE) && (me->super.sig >= (QSignal)Q_USER_SIG));
#ifdef Q_NASSERT
    (void)ctr; /* avoid compiler warning about unused variable */
#endif

    QF_CRIT_E_();
    me->ctr      = nTicks;   /* 设置计数器 */
    me->interval = interval; /* 设置周期间隔 */

    /* 判断时间事件是否未链接? */
    /* 注意: 在单个指定滴答速率的滴答周期中, 时间事件可能已解除激活但仍在列表中,
     * 因为解除链接仅在 QF_tickX() 函数中完成.
     */
    if ((me->super.refCtr_ & TE_IS_LINKED) == 0U) {
        me->super.refCtr_ |= TE_IS_LINKED; /* 标记为已链接 */

        /* 时间事件最初插入到"新激活"链表中, 该链表通过 QF_timeEvtHead_[tickRate].act 管理.
         * 之后, 在 QF_tickX() 函数内部, "新激活"列表会追加到主激活事件列表
         * (通过 QF_timeEvtHead_[tickRate].next 管理).
         * 这样做是为了保证对主列表的修改仅在 QF_tickX() 内部进行.
         */
        me->next                      = (QTimeEvt *)QF_timeEvtHead_[tickRate].act;
        QF_timeEvtHead_[tickRate].act = me;
    }

    QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_ARM, qs_id)
    QS_TIME_PRE_();        /* timestamp */
    QS_OBJ_PRE_(me);       /* this time event object */
    QS_OBJ_PRE_(me->act);  /* the active object */
    QS_TEC_PRE_(nTicks);   /* the number of ticks */
    QS_TEC_PRE_(interval); /* the interval */
    QS_U8_PRE_(tickRate);  /* tick rate */
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 解除(停用)时间事件, 使其可以安全地重复使用.
 *
 * @param[in,out] me 指向时间事件对象的指针
 *
 * @returns
 * 'true' 表示时间事件确实被停用, 即该事件当时正在运行.
 * 'false' 表示时间事件未被真正停用, \b 因为它本身未处于运行状态.
 * 这种 'false' 只可能出现在一次性时间事件(one-shot)已在到期时自动停用的情况下.
 * 对于这种情况, 返回 'false' 意味着时间事件已经被投递或发布, 并将在活动对象的状态机中被处理.
 *
 * @note
 * 对已经停用的时间事件再次调用解除激活是安全的, 不会产生副作用.
 */
bool QTimeEvt_disarm(QTimeEvt *const me)
{
    bool wasArmed;
#ifdef Q_SPY
    uint_fast8_t const qs_id = ((QActive *)(me->act))->prio;
#endif
    QF_CRIT_STAT_

    QF_CRIT_E_();

    /* 时间事件是否实际处于激活状态? */
    if (me->ctr != 0U) {
        wasArmed = true;
        me->super.refCtr_ |= TE_WAS_DISARMED;

        QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_DISARM, qs_id)
        QS_TIME_PRE_();            /* timestamp */
        QS_OBJ_PRE_(me);           /* this time event object */
        QS_OBJ_PRE_(me->act);      /* the target AO */
        QS_TEC_PRE_(me->ctr);      /* the number of ticks */
        QS_TEC_PRE_(me->interval); /* the interval */
        QS_U8_PRE_(me->super.refCtr_ & TE_TICK_RATE);
        QS_END_NOCRIT_PRE_()

        me->ctr = 0U; /* 标记从列表中移除 */
    } else {
        /* 时间事件已被自动停用 */
        wasArmed = false;
        me->super.refCtr_ &= (uint8_t)(~TE_WAS_DISARMED & 0xFFU); /* 清除停用标记 */

        QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_DISARM_ATTEMPT, qs_id)
        QS_TIME_PRE_();       /* timestamp */
        QS_OBJ_PRE_(me);      /* this time event object */
        QS_OBJ_PRE_(me->act); /* the target AO */
        QS_U8_PRE_(me->super.refCtr_ & TE_TICK_RATE);
        QS_END_NOCRIT_PRE_()
    }
    QF_CRIT_X_();
    return wasArmed;
}

/****************************************************************************/
/**
 * @brief
 * 重新启动(重新装载)时间事件, 使其以新的滴答数计数.
 * 该函数可用于 \b 调整周期性时间事件 的当前周期, \b 或防止一次性时间事件到期
 * (例如, 看门狗时间事件). 重新装载周期性时间事件不会改变其间隔,是调整时间事件相位的方便方法。
 *
 * @param[in,out] me     指向时间事件对象的指针
 * @param[in]     nTicks 用于重新装载时间事件的滴答数(按对应速率)
 *
 * @returns
 * 'true' 表示时间事件正在运行并已重新装载。
 * 'false' 表示时间事件未被真正重新装载, 因为它当时未运行.
 * 这种 'false' 只可能出现在一次性时间事件(one-shot)已自动停用的情况下.
 * 返回 'false' 表示事件已被投递或发布, 并将在活动对象的状态机中被处理.
 */
bool QTimeEvt_rearm(QTimeEvt *const me, QTimeEvtCtr const nTicks)
{
    uint_fast8_t tickRate = (uint_fast8_t)me->super.refCtr_ & TE_TICK_RATE;
    bool wasArmed;
#ifdef Q_SPY
    uint_fast8_t const qs_id = ((QActive *)(me->act))->prio;
#endif
    QF_CRIT_STAT_

    /** @pre AO必须有效, 滴答速率在范围内, nTicks不为零, 时间事件信号必须有效 */
    Q_REQUIRE_ID(600, (me->act != (void *)0) && (tickRate < QF_MAX_TICK_RATE) && (nTicks != 0U) && (me->super.sig >= (QSignal)Q_USER_SIG));

    QF_CRIT_E_();

    /* 时间事件未运行? */
    if (me->ctr == 0U) {
        wasArmed = false;

        /* NOTE: 在单个滴答周期内, 时间事件可能已被停用但仍链接在列表中,因为解除链接操作仅在 QF_tickX() 中执行 */
        /* 时间事件是否已链接? */
        if ((me->super.refCtr_ & TE_IS_LINKED) == 0U) {
            me->super.refCtr_ |= TE_IS_LINKED; /* mark as linked */

            /* 时间事件最初插入到基于 QF_timeEvtHead_[tickRate].act 的"新装载"列表中,
             * 之后在 QF_tickX() 中, "新装载"列表会被追加到主时间事件列表中,
             * 以保持对主列表的修改仅发生在 QF_tickX() 中.
             */
            me->next                      = (QTimeEvt *)QF_timeEvtHead_[tickRate].act;
            QF_timeEvtHead_[tickRate].act = me;
        }
    } else { /* 时间事件已被激活 */
        wasArmed = true;
    }
    me->ctr = nTicks; /* 重新装载滴答计数器(调整相位) */

    QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_REARM, qs_id)
    QS_TIME_PRE_();            /* timestamp */
    QS_OBJ_PRE_(me);           /* this time event object */
    QS_OBJ_PRE_(me->act);      /* the target AO */
    QS_TEC_PRE_(me->ctr);      /* the number of ticks */
    QS_TEC_PRE_(me->interval); /* the interval */
    QS_2U8_PRE_(tickRate, (wasArmed ? 1U : 0U));
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();
    return wasArmed;
}

/****************************************************************************/
/**
 * @brief
 * 用于检查一次性时间事件(one-shot)在 QTimeEvt_disarm() 操作中是否被真正停用.
 *
 * @param[in,out] me   指向时间事件对象的指针
 *
 * @returns
 * 'true' 表示时间事件在上一次 QTimeEvt_disarm() 操作中确实被停用.
 * 'false' 表示时间事件未真正被停用, 因为当时它并未运行.
 * 这种 'false' 只可能出现在一次性时间事件自动停用后到期的情况.
 * 在这种情况下, 返回 'false' 表示时间事件已经被投递或发布,
 * 并将在活动对象的事件队列中被处理.
 *
 * @note
 * 该函数有一个 \b 副作用: 设置"已停用(was disarmed)"状态,
 * 这意味着第二次及以后调用该函数时, 函数将返回 'true'.
 */
bool QTimeEvt_wasDisarmed(QTimeEvt *const me)
{
    uint8_t wasDisarmed = (me->super.refCtr_ & TE_WAS_DISARMED); /* 读取已停用标志 */
    me->super.refCtr_ |= TE_WAS_DISARMED;                        /* 设置已停用标志 */
    return (wasDisarmed != 0U) ? true : false;
}

/****************************************************************************/
/**
 * @brief
 * 用于检查时间事件在其关联的滴答速率下, 还剩多少个时钟滴答才会到期.
 *
 * @param[in,out] me   指向时间事件对象的指针
 *
 * @returns
 * 对于已激活的时间事件, 函数返回该时间事件当前的倒计数值.
 * 如果时间事件未激活, 则返回 0.
 *
 * @note
 * 该函数是线程安全的.
 */
QTimeEvtCtr QTimeEvt_currCtr(QTimeEvt const *const me)
{
    QTimeEvtCtr ret;
    QF_CRIT_STAT_

    QF_CRIT_E_();
    ret = me->ctr;
    QF_CRIT_X_();

    return ret;
}
