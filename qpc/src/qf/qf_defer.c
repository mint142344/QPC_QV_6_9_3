/**
 * @file
 * @brief QActive_defer() and QActive_recall() implementation.
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

Q_DEFINE_THIS_MODULE("qf_defer")

/****************************************************************************/
/**
 * @brief
 * 该函数是事件延迟(defer)支持的一部分, 一个主动对象可以使用此函数
 * 将事件 @p e 延迟到 QF 支持的原生事件队列 @p eq 中, QF 会正确处理事件的
 * 另一个未完成的引用, 并且不会在 RTC 步骤结束时回收该事件. 稍后, 主动对象
 * 可以一次从队列中取回一个事件.
 *
 * @param[in,out] me  指向主动对象的指针
 * @param[in]     eq  指向要从中取回事件的"原始"线程安全队列
 * @param[in]     e   指向要延迟的事件
 *
 * @returns
 * 当事件成功被延迟时返回 'true', 如果由于队列溢出而无法延迟事件, 则返回 'false'.
 *
 * 主动对象可以使用多个事件队列来延迟不同类型的事件.
 */
bool QActive_defer(QActive const *const me, QEQueue *const eq,
                   QEvt const *const e)
{
    bool status = QEQueue_post(eq, e, 0U, me->prio);
    QS_CRIT_STAT_

        (void)
    me; /* unused parameter */

    QS_BEGIN_PRE_(QS_QF_ACTIVE_DEFER, me->prio)
    QS_TIME_PRE_();                      /* 时间戳 */
    QS_OBJ_PRE_(me);                     /* 当前主动对象 */
    QS_OBJ_PRE_(eq);                     /* 延迟队列 */
    QS_SIG_PRE_(e->sig);                 /* 事件的信号 */
    QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
    QS_END_PRE_()

    return status;
}

/****************************************************************************/
/**
 * @brief
 * 该函数是事件延迟(defer)支持的一部分. 主动对象可以使用此函数
 * 从指定的 QF 事件队列中取回一个延迟事件. 取回事件意味着将其从延迟
 * 队列 @p eq 中移除, 并以 LIFO(后进先出)的方式投递到主动对象的事件队列中.
 *
 * @param[in,out] me  指向主动对象的指针
 * @param[in]     eq  指向要从中取回事件的"原始"线程安全队列
 *
 * @returns
 * 如果成功取回事件返回 'true', 否则返回 'false'.
 *
 * @note
 * 一个主动对象可以使用多个事件队列来延迟不同类型的事件.
 */
bool QActive_recall(QActive *const me, QEQueue *const eq)
{
    QEvt const *e = QEQueue_get(eq, me->prio);
    bool recalled;

    /* event available? */
    if (e != (QEvt *)0) {
        QF_CRIT_STAT_

        QACTIVE_POST_LIFO(me, e); /* 将事件投递到 AO 队列前端 */

        QF_CRIT_E_();

        /* 是否是动态事件 */
        if (e->poolId_ != 0U) {

            /* 投递到 AO 队列后, 事件的引用计数必须至少为 2;
             * 一次在延迟事件队列中(eq->get() 并未减少引用计数),
             * 一次在 AO 的事件队列中.
             */
            Q_ASSERT_CRIT_(210, e->refCtr_ >= 2U);

            /* 需要将引用计数减 1, 以反映从延迟队列中移除事件 */
            QF_EVT_REF_CTR_DEC_(e); /* decrement the reference counter */
        }

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_RECALL, me->prio)
        QS_TIME_PRE_();                      /* 时间戳 */
        QS_OBJ_PRE_(me);                     /* 当前主动对象 */
        QS_OBJ_PRE_(eq);                     /* 延迟队列 */
        QS_SIG_PRE_(e->sig);                 /* 事件信号 */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
        QS_END_NOCRIT_PRE_()

        QF_CRIT_X_();
        recalled = true;
    } else {
        QS_CRIT_STAT_

        QS_BEGIN_PRE_(QS_QF_ACTIVE_RECALL_ATTEMPT, me->prio)
        QS_TIME_PRE_();  /* 时间戳 */
        QS_OBJ_PRE_(me); /* 当前主动对象 */
        QS_OBJ_PRE_(eq); /* 延迟队列 */
        QS_END_PRE_()

        recalled = false;
    }
    return recalled;
}

/****************************************************************************/
/**
 * @brief
 * 该函数是事件延迟支持的一部分. 主动对象可以使用此函数
 * 清空指定的 QF 事件队列. 该函数确保事件不会被泄漏.
 *
 * @param[in,out] me  指向主动对象的指针
 * @param[in]     eq  指向要清空的"原始"线程安全队列
 *
 * @returns
 * 实际从队列中清空的事件数量.
 */
uint_fast16_t QActive_flushDeferred(QActive const *const me,
                                    QEQueue *const eq)
{
    QEvt const *e   = QEQueue_get(eq, me->prio);
    uint_fast16_t n = 0U;

    (void)me; /* unused parameter */

    for (; e != (QEvt *)0; e = QEQueue_get(eq, me->prio)) {
        QF_gc(e); /* 垃圾回收事件 */
        ++n;      /* 统计已清空的事件数量 */
    }
    return n;
}
