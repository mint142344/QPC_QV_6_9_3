/**
 * @file
 * @brief Publish-Subscribe services
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.3
 * Last updated on  2021-02-26
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

Q_DEFINE_THIS_MODULE("qf_ps")

/* Package-scope objects ****************************************************/
QSubscrList *QF_subscrList_;
enum_t QF_maxPubSignal_;

/****************************************************************************/
/**
 * @brief
 * 此函数用于初始化 QF 的发布-订阅机制. 必须在应用程序中发生任何订阅或发布操作之前 \b 且仅调用一次
 *
 * @param[in] subscrSto 指向订阅者列表数组的指针
 * @param[in] maxSignal 订阅者数组的长度, 同时也是能够发布或订阅的最大信号值
 *
 * 订阅者列表数组通过信号进行索引, 用于建立信号和订阅者列表之间的映射关系.
 * 每个订阅者列表(::QSubscrList 类型)是一个位掩码, 其中每一位对应一个活动对象的唯一优先级.
 * ::QSubscrList 位掩码的大小取决于宏 #QF_MAX_ACTIVE 的值.
 *
 * @note
 * 发布-订阅机制是可选的. 如果你选择不使用发布-订阅功能,
 * 那么就不需要调用 QF_psInit(), 也不需要为订阅者列表分配内存.
 */
void QF_psInit(QSubscrList *const subscrSto, enum_t const maxSignal)
{
    QF_subscrList_   = subscrSto;
    QF_maxPubSignal_ = maxSignal;

    /* 将订阅者列表清零, 这样即使启动代码没有按照 C 标准要求
     * 正确清除未初始化数据, 框架仍然可以正常运行.
     */
    QF_bzero(subscrSto, (uint_fast16_t)maxSignal * sizeof(QSubscrList));
}

/****************************************************************************/
/**
 * @brief
 * 此函数将事件 @a e 按照 \b FIFO 策略 投递给所有
 * 订阅了该事件信号 @a e->sig 的活动对象, 这个过程称为 \b 多播(multicasting).
 * 本函数的多播机制基于事件内部的引用计数, 因此效率非常高, 并且实现了"零拷贝"的事件多播.
 * 该函数可以在系统的任何位置调用, 包括中断服务程序 (ISR), 设备驱动程序, 以及活动对象内部.
 *
 * @note
 * 为了避免投递到 AO 队列中的事件发生意外的重新排序, 多播过程中调度器会被 \b 锁定.
 * 不过, 调度器只会被锁定到订阅者中的最高优先级, 因此未订阅该事件的更高优先级 AO 不会受到影响.
 *
 * @attention
 * 此函数应当仅通过宏 QF_PUBLISH() 调用.
 */
#ifndef Q_SPY
void QF_publish_(QEvt const *const e)
#else
void QF_publish_(QEvt const *const e,
                 void const *const sender, uint_fast8_t const qs_id)
#endif
{
    QPSet subscrList; /* 订阅者列表的本地可修改副本 */
    QF_CRIT_STAT_

    /** @pre 发布的信号必须在配置范围内 */
    Q_REQUIRE_ID(200, e->sig < (QSignal)QF_maxPubSignal_);

    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_PUBLISH, qs_id)
    QS_TIME_PRE_();                      /* the timestamp */
    QS_OBJ_PRE_(sender);                 /* the sender object */
    QS_SIG_PRE_(e->sig);                 /* the signal of the event */
    QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
    QS_END_NOCRIT_PRE_()

    /* 动态事件? */
    if (e->poolId_ != 0U) {
        /** @note 在多播过程中, 动态事件的引用计数会被增加, 以防止事件被过早回收.
         * 在函数结束时, 垃圾回收器 (QF_gc()) 会减少引用计数, 并在计数归零时回收事件.
         * 这也覆盖了"事件发布时没有订阅者"的情况.
         */
        QF_EVT_REF_CTR_INC_(e);
    }

    /* 复制订阅者列表的本地可修改副本 */
    subscrList = QF_PTR_AT_(QF_subscrList_, e->sig);
    QF_CRIT_X_();

    if (QPSet_notEmpty(&subscrList)) { /* 有订阅者? */
        uint_fast8_t p;
        QF_SCHED_STAT_

        QPSet_findMax(&subscrList, p); /* 找到最高优先级订阅者 */

        QF_SCHED_LOCK_(p); /* 锁定调度器到优先级 p */
        do {               /* 遍历所有订阅者 */
            /* AO 的优先级必须已经在框架中注册 */
            Q_ASSERT_ID(210, QF_active_[p] != (QActive *)0);

            /* QACTIVE_POST() 内部会断言以防队列溢出 */
            QACTIVE_POST(QF_active_[p], e, sender);

            QPSet_remove(&subscrList, p);      /* 移除已处理的订阅者 */
            if (QPSet_notEmpty(&subscrList)) { /* 还有订阅者吗? */
                QPSet_findMax(&subscrList, p); /* 找下一个最高优先级订阅者 */
            } else {
                p = 0U; /* 没有更多订阅者 */
            }
        } while (p != 0U);
        QF_SCHED_UNLOCK_(); /* 解锁调度器 */
    }

    /* 垃圾回收: 减少引用计数并在归零时回收事件.  这同时适用于"有订阅者"和"无订阅者"的情况 */
    QF_gc(e);
}

/****************************************************************************/
/**
 * @brief
 * 本函数是 QF 中 发布-订阅 (Publish-Subscribe) 事件传递机制的一部分.
 * 订阅某个事件意味着; 框架会开始将所有具有指定信号 @p sig 的已发布事件,
 * 投递到活动对象 @p me 的事件队列中.
 *
 * @param[in,out] me  指针
 * @param[in]     sig 要订阅的事件信号
 */
void QActive_subscribe(QActive const *const me, enum_t const sig)
{
    uint_fast8_t p = (uint_fast8_t)me->prio;
    QF_CRIT_STAT_

    Q_REQUIRE_ID(300, ((enum_t)Q_USER_SIG <= sig) && (sig < QF_maxPubSignal_) && (0U < p) && (p <= QF_MAX_ACTIVE) && (QF_active_[p] == me));

    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_SUBSCRIBE, me->prio)
    QS_TIME_PRE_();   /* timestamp */
    QS_SIG_PRE_(sig); /* the signal of this event */
    QS_OBJ_PRE_(me);  /* this active object */
    QS_END_NOCRIT_PRE_()

    /* set the priority bit */
    QPSet_insert(&QF_PTR_AT_(QF_subscrList_, sig), p);

    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 本函数是 QF 中 发布-订阅 (Publish-Subscribe) 事件传递机制的一部分.
 * 取消订阅某个事件意味着; 框架将不再把具有指定信号 @p sig 的已发布事件,
 * 投递到活动对象 @p me 的事件队列中.
 *
 * @param[in] me  指针
 * @param[in] sig 要取消订阅的事件信号
 *
 * @note
 * 由于事件队列存在延迟, 活动对象在取消订阅某个信号 @p sig 后,
 * 不能假设该信号一定不会再被分发到其状态机.
 * 因为事件可能已经在队列中, 或者正要被投递, 而取消订阅操作不会清除这些事件.
 *
 * @note
 * 如果尝试取消订阅一个从未订阅过的信号, 会被视为错误, QF 将触发断言.
 */
void QActive_unsubscribe(QActive const *const me, enum_t const sig)
{
    uint_fast8_t p = (uint_fast8_t)me->prio;
    QF_CRIT_STAT_

    /** @pre 信号和优先级必须在合法范围内，且该活动对象必须已在框架中注册 */
    Q_REQUIRE_ID(400, ((enum_t)Q_USER_SIG <= sig) && (sig < QF_maxPubSignal_) && (0U < p) && (p <= QF_MAX_ACTIVE) && (QF_active_[p] == me));

    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_UNSUBSCRIBE, me->prio)
    QS_TIME_PRE_();   /* timestamp */
    QS_SIG_PRE_(sig); /* the signal of this event */
    QS_OBJ_PRE_(me);  /* this active object */
    QS_END_NOCRIT_PRE_()

    /* clear priority bit */
    QPSet_remove(&QF_PTR_AT_(QF_subscrList_, sig), p);

    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 本函数是 QF 中 发布-订阅 (Publish-Subscribe) 事件传递机制的一部分.
 * 从所有事件中取消订阅意味着; 框架将不再把任何已发布的事件
 * 投递到活动对象 @p me 的事件队列中.
 *
 * @param[in] me  指针
 *
 * @note
 * 由于事件队列存在延迟, 活动对象在取消订阅所有事件后,
 * 不能假设它的状态机永远不会再接收到任何事件.
 * 事件可能已经在队列中, 或者正要被投递, 而取消订阅操作不会清除这些事件.
 * 此外, 其他事件传递机制(例如 直接事件投递 或 定时事件)，
 * 仍然可能将事件发送到该活动对象的事件队列.
 */
void QActive_unsubscribeAll(QActive const *const me)
{
    uint_fast8_t p = (uint_fast8_t)me->prio;
    enum_t sig;

    Q_REQUIRE_ID(500, (0U < p) && (p <= QF_MAX_ACTIVE) && (QF_active_[p] == me));

    for (sig = (enum_t)Q_USER_SIG; sig < QF_maxPubSignal_; ++sig) {
        QF_CRIT_STAT_
        QF_CRIT_E_();
        if (QPSet_hasElement(&QF_PTR_AT_(QF_subscrList_, sig), p)) {
            QPSet_remove(&QF_PTR_AT_(QF_subscrList_, sig), p);

            QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_UNSUBSCRIBE, me->prio)
            QS_TIME_PRE_();   /* timestamp */
            QS_SIG_PRE_(sig); /* the signal of this event */
            QS_OBJ_PRE_(me);  /* this active object */
            QS_END_NOCRIT_PRE_()
        }
        QF_CRIT_X_();

        /* prevent merging critical sections */
        QF_CRIT_EXIT_NOP();
    }
}
