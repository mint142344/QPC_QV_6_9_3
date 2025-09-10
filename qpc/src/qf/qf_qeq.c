/**
 * @file
 * @brief ::QEQueue implementation (QP native thread-safe queue)
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

Q_DEFINE_THIS_MODULE("qf_qeq")

/****************************************************************************/
/**
 * @brief
 * 通过提供环形缓冲区的存储空间, 初始化事件队列.
 *
 * @param[in,out] me   指针
 * @param[in]     qSto 指向 ::QEvt 指针数组的指针, 用作事件队列的环形缓冲区
 * @param[in]     qLen @p qSto 缓冲区的长度(单位是 ::QEvt 指针个数)
 *
 * @note 队列的实际容量是 qLen + 1, 因为额外预留了一个 frontEvt 的位置.
 *
 * @note
 * 这个函数也用于在内建的 QV 和 QK 内核中初始化活动对象的事件队列,
 * 以及其他 QP 移植到的 OS/RTOS(那些没有提供合适消息队列的情况)
 */
void QEQueue_init(QEQueue *const me, QEvt const **const qSto,
                  uint_fast16_t const qLen)
{
    me->frontEvt = (QEvt *)0;        /* 队列里还没有事件 */
    me->ring     = qSto;             /* 指向环形缓冲区的起始地址 */
    me->end      = (QEQueueCtr)qLen; /* 环形缓冲区的大小 */

    if (qLen != 0U) {
        me->head = 0U; /* 入队指针从 0 开始 */
        me->tail = 0U; /* 出队指针从 0 开始 */
    }

    me->nFree = (QEQueueCtr)(qLen + 1U); /* 队列的空闲单元数 (+1 表示包含 frontEvt) */
    me->nMin  = me->nFree;               /* 最小空闲单元数初始化为最大值 */
}

/**
 * @note
 * 这个函数用于"原始(raw)"线程安全队列, 而不是用于活动对象(active object)的队列.
 */

/****************************************************************************/
/**
 * @brief
 * 按照先进先出(FIFO)的顺序, 将一个事件投递到"原始(raw)"线程安全事件队列中.
 *
 * @param[in,out] me     指针
 * @param[in]     e      指向要投递到队列的事件的指针
 * @param[in]     margin 事件投递之后, 队列中必须保留的最少空闲槽数.
 *                       特殊值 #QF_NO_MARGIN 表示如果事件无法投递, 则直接触发断言.
 *
 * @note
 * 当 @p margin 参数为 #QF_NO_MARGIN 时, 表示假定投递操作一定会成功
 * (事件投递保证). 如果事件无法投递, 这种情况下会触发断言.
 *
 * @returns
 * - 返回 `true` : 表示投递成功, 满足给定的 margin 要求;
 * - 返回 `false`: 表示投递失败.
 *
 * @note
 * 这个函数既可以从任务上下文调用, 也可以从中断服务例程(ISR)上下文调用.
 */
bool QEQueue_post(QEQueue *const me, QEvt const *const e,
                  uint_fast16_t const margin, uint_fast8_t const qs_id)
{
    QEQueueCtr nFree; /* 临时变量, 用于保存可用空间, 避免对 volatile 的重复访问 */
    bool status;
    QF_CRIT_STAT_

    /* @pre 前提条件: 事件必须有效 */
    Q_REQUIRE_ID(200, e != (QEvt *)0);

    (void)qs_id; /* unused parameter (outside Q_SPY build configuration) */

    QF_CRIT_E_();      /* 进入临界区 */
    nFree = me->nFree; /* 取出当前队列空闲槽数 */

    /* 判断是否满足 margin 要求 */
    if (((margin == QF_NO_MARGIN) && (nFree > 0U)) || (nFree > (QEQueueCtr)margin)) {
        /* 如果是动态事件(从事件池分配的) */
        if (e->poolId_ != 0U) {
            QF_EVT_REF_CTR_INC_(e); /* 增加事件的引用计数 */
        }

        --nFree;           /* 占用一个空闲槽 */
        me->nFree = nFree; /* 更新队列的空闲槽计数 */
        if (me->nMin > nFree) {
            me->nMin = nFree; /* 更新最小空闲数(用于统计队列使用峰值) */
        }

        QS_BEGIN_NOCRIT_PRE_(QS_QF_EQUEUE_POST, qs_id)
        QS_TIME_PRE_();                      /* timestamp */
        QS_SIG_PRE_(e->sig);                 /* the signal of this event */
        QS_OBJ_PRE_(me);                     /* this queue object */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_EQC_PRE_(nFree);                  /* number of free entries */
        QS_EQC_PRE_(me->nMin);               /* min number of free entries */
        QS_END_NOCRIT_PRE_()

        /* 队列是否为空? */
        if (me->frontEvt == (QEvt *)0) {
            me->frontEvt = e; /* 队列为空, 事件直接放到 frontEvt */
        } else {
            /* 队列非空, 将事件插入环形缓冲区(FIFO) */
            QF_PTR_AT_(me->ring, me->head) = e; /* 插入事件 */
            /* 如果 head 到头了, 需要回绕 */
            if (me->head == 0U) {
                me->head = me->end; /* wrap around */
            }
            --me->head;
        }
        status = true; /* 投递成功 */
    } else {
        /** @note margin 要求不满足
         * 如果 margin == QF_NO_MARGIN, 表示不能丢事件, 这里会触发断言
         */
        Q_ASSERT_CRIT_(210, margin != QF_NO_MARGIN);

        QS_BEGIN_NOCRIT_PRE_(QS_QF_EQUEUE_POST_ATTEMPT, qs_id)
        QS_TIME_PRE_();                      /* timestamp */
        QS_SIG_PRE_(e->sig);                 /* the signal of this event */
        QS_OBJ_PRE_(me);                     /* this queue object */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_EQC_PRE_(nFree);                  /* number of free entries */
        QS_EQC_PRE_(margin);                 /* margin requested */
        QS_END_NOCRIT_PRE_()

        status = false; /* 投递失败 */
    }
    QF_CRIT_X_();

    return status;
}

/****************************************************************************/
/**
 * @brief
 * 按照后进先出(LIFO)的顺序, 将一个事件投递到"原始(raw)"线程安全事件队列中.
 *
 * @param[in,out] me  指针
 * @param[in]     e   指向要投递到队列的事件的指针
 *
 * @attention
 * LIFO 策略必须非常小心使用, 因为它会改变队列中事件的顺序.
 *
 * @note
 * 这个函数可以从任何任务上下文或者 ISR(中断服务例程)上下文调用.
 *
 * @note
 * 这个函数用于"原始(raw)"线程安全队列, 不是用于活动对象(active object)的队列.
 */
void QEQueue_postLIFO(QEQueue *const me, QEvt const *const e,
                      uint_fast8_t const qs_id)
{
    QEvt const *frontEvt; /* 临时变量，用于避免对 volatile 的未定义行为访问 */
    QEQueueCtr nFree;     /* 临时变量，用于避免对 volatile 的未定义行为访问 */
    QF_CRIT_STAT_

        (void)
    qs_id; /* unused parameter (outside Q_SPY build configuration) */

    QF_CRIT_E_();
    nFree = me->nFree; /* 将 volatile 变量复制到临时变量 */

    /** @pre 队列必须能够接受事件(不能溢出) */
    Q_REQUIRE_CRIT_(300, nFree != 0U);

    /* 事件是否为动态事件? */
    if (e->poolId_ != 0U) {
        QF_EVT_REF_CTR_INC_(e); /* 增加引用计数器 */
    }

    --nFree;           /* 一个空闲槽被使用 */
    me->nFree = nFree; /* 更新 volatile 变量 */
    if (me->nMin > nFree) {
        me->nMin = nFree; /* 更新历史最小空闲数 */
    }

    QS_BEGIN_NOCRIT_PRE_(QS_QF_EQUEUE_POST_LIFO, qs_id)
    QS_TIME_PRE_();                      /* timestamp */
    QS_SIG_PRE_(e->sig);                 /* the signal of this event */
    QS_OBJ_PRE_(me);                     /* this queue object */
    QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count of the event */
    QS_EQC_PRE_(nFree);                  /* number of free entries */
    QS_EQC_PRE_(me->nMin);               /* min number of free entries */
    QS_END_NOCRIT_PRE_()

    frontEvt     = me->frontEvt; /* 将 volatile 变量读入临时变量 */
    me->frontEvt = e;            /* 将事件直接放到队列前端 */

    /* 队列非空? */
    if (frontEvt != (QEvt *)0) {
        ++me->tail;
        if (me->tail == me->end) { /* 是否需要环绕? */
            me->tail = 0U;         /* 环绕回队列开头 */
        }
        QF_PTR_AT_(me->ring, me->tail) = frontEvt; /* 保存旧的队列前端事件 */
    }

    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 从"原始(raw)"线程安全队列的前端获取一个事件, 并将其指针返回给调用者.
 *
 * @param[in,out] me  指针
 *
 * @returns
 * - 如果队列非空, 返回队列前端事件的指针;
 * - 如果队列为空, 返回 NULL.
 *
 * @note
 * 这个函数用于"原始(raw)"线程安全队列, 不是用于活动对象(active object)的队列.
 */
QEvt const *QEQueue_get(QEQueue *const me, uint_fast8_t const qs_id)
{
    QEvt const *e;
    QF_CRIT_STAT_

        (void)
    qs_id; /* unused parameter (outside Q_SPY build configuration) */

    QF_CRIT_E_();
    e = me->frontEvt; /* 始终从 frontEvt 移除事件 */

    /* 队列非空? */
    if (e != (QEvt *)0) {
        /* 使用临时变量增加 volatile me->nFree */
        QEQueueCtr nFree = me->nFree + 1U;
        me->nFree        = nFree; /* 更新空闲计数 */

        /* 队列环形缓冲区中还有事件? */
        if (nFree <= me->end) {
            me->frontEvt = QF_PTR_AT_(me->ring, me->tail); /* 从尾部获取事件 */
            if (me->tail == 0U) {                          /* 是否需要环绕？ */
                me->tail = me->end;                        /* 环绕到队列末尾 */
            }
            --me->tail;

            QS_BEGIN_NOCRIT_PRE_(QS_QF_EQUEUE_GET, qs_id)
            QS_TIME_PRE_();                      /* timestamp */
            QS_SIG_PRE_(e->sig);                 /* the signal of this event */
            QS_OBJ_PRE_(me);                     /* this queue object */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_EQC_PRE_(nFree);                  /* number of free entries */
            QS_END_NOCRIT_PRE_()
        } else {
            me->frontEvt = (QEvt *)0; /* 队列变为空 */

            /* 队列中所有条目都应该为空(+1 为 frontEvt) */
            Q_ASSERT_CRIT_(410, nFree == (me->end + 1U));

            QS_BEGIN_NOCRIT_PRE_(QS_QF_EQUEUE_GET_LAST, qs_id)
            QS_TIME_PRE_();                      /* timestamp */
            QS_SIG_PRE_(e->sig);                 /* the signal of this event */
            QS_OBJ_PRE_(me);                     /* this queue object */
            QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_END_NOCRIT_PRE_()
        }
    }
    QF_CRIT_X_();
    return e;
}
