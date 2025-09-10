/**
 * @file
 * @brief QF/C dynamic event management
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.3
 * Last updated on  2021-04-09
 *
 *                    Q u a n t u m  L e a P s
 *                    ------------------------
 *                    Modern Embedded Software
 *
 * Copyright (C) 2005-2021 Quantum Leaps, LLC. All rights reserved.
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

Q_DEFINE_THIS_MODULE("qf_dyn")

/* Package-scope objects ****************************************************/
QF_EPOOL_TYPE_ QF_pool_[QF_MAX_EPOOL]; /* 分配事件池 */
uint_fast8_t QF_maxPool_;              /* 已初始化的事件池数量 */

/****************************************************************************/
#ifdef Q_EVT_CTOR /* 是否提供 ::QEvt 类的构造函数? */

/**
 * @brief
 * 当宏 #Q_EVT_CTOR 被定义时, 提供 ::QEvt 类的构造函数.
 *
 * @param[in,out] me   指向事件对象的指针
 * @param[in]     sig  要分配给事件的信号
 */
QEvt *QEvt_ctor(QEvt *const me, enum_t const sig)
{
    /** @pre me 指针必须有效 */
    Q_REQUIRE_ID(100, me != (QEvt *)0);
    me->sig = (QSignal)sig;
    return me;
}

#endif /* Q_EVT_CTOR */

/****************************************************************************/
/**
 * @brief
 * 该函数用于一次初始化一个事件池, 并且在使用每个事件池之前必须对其调用一次.
 *
 * @param[in] poolSto  指向事件池存储区的指针
 * @param[in] poolSize 存储区大小(字节为单位)
 * @param[in] evtSize  池中每个块的大小(字节为单位), 决定了从该池中分配的事件的最大尺寸.
 *
 * @attention
 * 可以通过多次连续调用 QF_poolInit() 初始化多个事件池. 然而, 为了内部实现的简化,
 * 必须按照事件尺寸 \b 升序 初始化事件池.
 *
 * 许多 RTOS 提供固定块大小的堆(也称为内存池), 可以用作 QF 事件池. 如果缺少此类支持,
 * QF 提供了原生的 QF 事件池实现. 宏 #QF_EPOOL_TYPE_ 决定了特定 QF 端口使用的事件池类型.
 * 详情参见结构体 ::QMPool。
 *
 * @note 实际可用的事件数量可能小于 ( @p poolSize / @p evtSize), 因为池内部可能会对块进行对齐,
 * 可以通过调用 QF_getPoolMin() 检查池的容量.
 *
 * @note 动态事件分配是可选的, 即可以选择不使用动态事件. 在这种情况下,
 * 调用 QF_poolInit() 并使用内存块占用内存是不必要的.
 */
void QF_poolInit(void *const poolSto, uint_fast32_t const poolSize,
                 uint_fast16_t const evtSize)
{
    /** @pre 不能超过可用内存池的数量 */
    Q_REQUIRE_ID(200, QF_maxPool_ < Q_DIM(QF_pool_));
    /** @pre 请按 evtSize 的升序初始化事件池 */
    Q_REQUIRE_ID(201, (QF_maxPool_ == 0U) || (QF_EPOOL_EVENT_SIZE_(QF_pool_[QF_maxPool_ - 1U]) < evtSize));

    /* 执行平台相关的事件池初始化 */
    QF_EPOOL_INIT_(QF_pool_[QF_maxPool_], poolSto, poolSize, evtSize);
    ++QF_maxPool_; /* 池数量加一 */

#ifdef Q_SPY
    /* 为已初始化的池生成对象字典条目 */
    {
        char_t obj_name[9] = "EvtPool?";
        obj_name[7]        = '0' + (QF_maxPool_ & 0x7FU);
        QS_obj_dict_pre_(&QF_pool_[QF_maxPool_ - 1U], obj_name);
    }
#endif /* Q_SPY*/
}

/****************************************************************************/
/**
 * @brief
 * 从 QF 事件池中动态分配一个事件.
 *
 * @param[in] evtSize  要分配事件的大小(字节)
 * @param[in] margin   分配完成后, 事件池中仍然可用的未分配事件数.
 *                     特殊值 #QF_NO_MARGIN 表示如果分配失败则触发断言.
 * @param[in] sig      分配的事件要设置的信号
 *
 * @returns
 * 指向新分配事件的指针. 只有在 margin != #QF_NO_MARGIN 且无法在给定池中
 * 保留指定 margin 的情况下分配事件时, 该指针才可能为 NULL.
 *
 * @note
 * 内部 QF 函数 QF_newX_() 会在 margin 参数为 #QF_NO_MARGIN 且由于事件池耗尽
 * 或请求事件大小过大导致无法分配事件时触发断言.
 *
 * @note
 * 应用程序代码不应直接调用此函数. 唯一允许的使用方式是通过宏 Q_NEW() 或 Q_NEW_X().
 */
QEvt *QF_newX_(uint_fast16_t const evtSize,
               uint_fast16_t const margin, enum_t const sig)
{
    QEvt *e;
    uint_fast8_t idx;
    QS_CRIT_STAT_

    /* 找到能满足请求事件大小的事件池索引 */
    for (idx = 0U; idx < QF_maxPool_; ++idx) {
        if (evtSize <= QF_EPOOL_EVENT_SIZE_(QF_pool_[idx])) {
            break;
        }
    }
    /* 不允许超出已注册池的数量 */
    Q_ASSERT_ID(310, idx < QF_maxPool_);

    /* 获取事件 -- 平台相关 */
#ifdef Q_SPY
    QF_EPOOL_GET_(QF_pool_[idx], e,
                  ((margin != QF_NO_MARGIN) ? margin : 0U),
                  (uint_fast8_t)QS_EP_ID + idx + 1U);
#else
    QF_EPOOL_GET_(QF_pool_[idx], e,
                  ((margin != QF_NO_MARGIN) ? margin : 0U), 0U);
#endif

    /* 事件是否成功分配 */
    if (e != (QEvt *)0) {
        e->sig     = (QSignal)sig;        /* 设置事件信号 */
        e->poolId_ = (uint8_t)(idx + 1U); /* 存储事件池 ID */
        e->refCtr_ = 0U;                  /* 设置引用计数为 0 */

        QS_BEGIN_PRE_(QS_QF_NEW, (uint_fast8_t)QS_EP_ID + e->poolId_)
        QS_TIME_PRE_();       /* 时间戳 */
        QS_EVS_PRE_(evtSize); /* 事件大小 */
        QS_SIG_PRE_(sig);     /* 事件信号 */
        QS_END_PRE_()
    } else {
        /* 该断言表示事件分配失败, 此类失败不可容忍.
         * 最常见的原因是应用程序中存在事件泄漏.
         */
        Q_ASSERT_ID(320, margin != QF_NO_MARGIN);

        QS_BEGIN_PRE_(QS_QF_NEW_ATTEMPT, (uint_fast8_t)QS_EP_ID + idx + 1U)
        QS_TIME_PRE_();       /* 时间戳 */
        QS_EVS_PRE_(evtSize); /* 事件大小 */
        QS_SIG_PRE_(sig);     /* 事件信号 */
        QS_END_PRE_()
    }
    return e; /* 如果无法容忍分配失败, 则不能为 NULL */
}

/****************************************************************************/
/**
 * @brief
 * 此函数实现了动态事件的简单垃圾回收机制.
 * 只有动态事件才有资格被回收.(动态事件是从事件池中分配的事件, 其 e->poolId_ 属性非零)
 * 函数会先将事件的引用计数 e->refCtr_ 减 1, 只有当计数降到 0 时(意味着没有任何引用指向此事件)
 * 才会将事件回收. 回收的方式是将动态事件返回到它最初分配的事件池中.
 *
 * @param[in] e  指向要回收的事件的指针
 *
 * @note
 * QF 会在所有适当的上下文中自动调用垃圾回收器(自动垃圾回收),
 * 因此应用程序通常不需要直接调用 QF_gc()。
 * QF_gc() 仅用于特殊情况, 例如应用程序将动态事件发送到 "原始" 线程安全队列 (::QEQueue) 时.
 * 对这些队列的事件, 自动垃圾回收 \b 不会 进行, 此时需要显式调用 QF_gc().
 */
void QF_gc(QEvt const *const e)
{
    /* 是否为动态事件 */
    if (e->poolId_ != 0U) {
        QF_CRIT_STAT_
        QF_CRIT_E_();

        /* 不是最后一个引用? */
        if (e->refCtr_ > 1U) {

            QS_BEGIN_NOCRIT_PRE_(QS_QF_GC_ATTEMPT,
                                 (uint_fast8_t)QS_EP_ID + e->poolId_)
            QS_TIME_PRE_();                      /* 时间戳 */
            QS_SIG_PRE_(e->sig);                 /* 事件信号 */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 事件池 ID 和引用计数 */
            QS_END_NOCRIT_PRE_()

            QF_EVT_REF_CTR_DEC_(e); /* 引用计数减 1 */

            QF_CRIT_X_();
        } else {
            /* 这是该事件的最后一个引用, 回收它 */
            uint_fast8_t idx = (uint_fast8_t)e->poolId_ - 1U;

            QS_BEGIN_NOCRIT_PRE_(QS_QF_GC,
                                 (uint_fast8_t)QS_EP_ID + e->poolId_)
            QS_TIME_PRE_();                      /* 时间戳 */
            QS_SIG_PRE_(e->sig);                 /* 事件信号 */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 事件池 ID 和引用计数 */
            QS_END_NOCRIT_PRE_()

            QF_CRIT_X_();

            /* 事件池 ID 必须在有效范围内 */
            Q_ASSERT_ID(410, idx < QF_maxPool_);

            /* 去掉 const 限定符, 这是安全的, 因为事件来自事件池 */
#ifdef Q_SPY
            QF_EPOOL_PUT_(QF_pool_[idx], QF_EVT_CONST_CAST_(e),
                          (uint_fast8_t)QS_EP_ID + e->poolId_);
#else
            QF_EPOOL_PUT_(QF_pool_[idx], QF_EVT_CONST_CAST_(e), 0U);
#endif
        }
    }
}

/****************************************************************************/
/**
 * @brief
 * 创建并返回对当前事件 e 的一个新引用
 *
 * @param[in] e       指向当前事件的指针
 * @param[in] evtRef  事件引用
 *
 * @returns  新创建的对事件 @p e 的引用
 *
 * @note
 * 应用程序不应直接调用此函数.
 * 唯一允许的使用方式是通过宏 Q_NEW_REF().
 */
QEvt const *QF_newRef_(QEvt const *const e, void const *const evtRef)
{
    QF_CRIT_STAT_

    /*! @pre 事件必须是动态事件, 且提供的事件引用必须尚未使用 */
    Q_REQUIRE_ID(500,
                 (e->poolId_ != 0U) && (evtRef == (void *)0));

    QF_CRIT_E_();

    QF_EVT_REF_CTR_INC_(e); /* 增加引用计数 */

    QS_BEGIN_NOCRIT_PRE_(QS_QF_NEW_REF,
                         (uint_fast8_t)QS_EP_ID + e->poolId_)
    QS_TIME_PRE_();                      /* 时间戳 */
    QS_SIG_PRE_(e->sig);                 /* 事件信号 */
    QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 事件池 ID 和引用计数 */
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();

    return e;
}
/****************************************************************************/
/**
 * @brief
 * 删除对事件 e 的一个已存在引用
 *
 * @param[in] evtRef  事件引用
 *
 * @note
 * 应用程序不应直接调用此函数.
 * 唯一允许的使用方式是通过宏 Q_DELETE_REF().
 */
void QF_deleteRef_(void const *const evtRef)
{
    QS_CRIT_STAT_
    QEvt const *const e = (QEvt const *)evtRef;

    QS_BEGIN_PRE_(QS_QF_DELETE_REF,
                  (uint_fast8_t)QS_EP_ID + e->poolId_)
    QS_TIME_PRE_();                      /* 时间戳 */
    QS_SIG_PRE_(e->sig);                 /* 事件信号 */
    QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 事件池 ID 和引用计数 */
    QS_END_PRE_()

    QF_gc(e);
}

/****************************************************************************/
/**
 * @brief
 * 获取任何已注册事件池的最大块大小
 */
uint_fast16_t QF_poolGetMaxBlockSize(void)
{
    return QF_EPOOL_EVENT_SIZE_(QF_pool_[QF_maxPool_ - 1U]);
}
