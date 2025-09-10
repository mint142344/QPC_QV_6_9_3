/**
 * @file
 * @brief ::QActive native queue operations (based on ::QEQueue)
 *
 * @note
 * this source file is only included in the application build when the native
 * QF active object queue is used (instead of a message queue of an RTOS).
 *
 * @ingroup qf
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

Q_DEFINE_THIS_MODULE("qf_actq")

/****************************************************************************/
#ifdef Q_SPY
/**
 * @brief
 * 直接事件投递是 QF 中最简单的异步通信方式.
 *
 * @param[in,out] me     指针
 * @param[in]     e      指向要投递的事件的指针
 * @param[in]     margin 投递事件后队列中所需的空闲槽数量.
 *                       特殊值 #QF_NO_MARGIN 表示如果投递失败则触发断言.
 * @param[in]     sender 发送者对象指针(仅用于 QS 跟踪)
 *
 * @returns
 * 如果投递成功(满足提供的 margin) 返回 'true', 投递失败返回 'false'.
 *
 * @attention
 * 该函数应仅通过宏 QACTIVE_POST() 或 QACTIVE_POST_X() 调用.
 *
 * @note
 * @p margin 参数的 #QF_NO_MARGIN 值表示期望投递操作成功(保证事件投递).
 * 如果事件在此情况下无法投递，将触发断言.
 */
bool QActive_post_(QActive *const me, QEvt const *const e,
                   uint_fast16_t const margin, void const *const sender)
#else
bool QActive_post_(QActive *const me, QEvt const *const e,
                   uint_fast16_t const margin)
#endif
{
    QEQueueCtr nFree; /* 临时变量, 用于避免 volatile 访问的未定义行为 */
    bool status;
    QF_CRIT_STAT_
    QS_TEST_PROBE_DEF(&QActive_post_)

    /** @pre 事件指针必须有效 */
    Q_REQUIRE_ID(100, e != (QEvt *)0);

    QF_CRIT_E_();
    nFree = me->eQueue.nFree; /* 将 volatile 变量复制到临时变量 */

    /* 测试探针#1; 模拟队列溢出 */
    QS_TEST_PROBE_ID(1,
                     nFree = 0U;)

    if (margin == QF_NO_MARGIN) {
        if (nFree > 0U) {
            status = true; /* 可以投递 */
        } else {
            status = false;     /* 无法投递 */
            Q_ERROR_CRIT_(110); /* 必须能够投递事件 */
        }
    } else if (nFree > (QEQueueCtr)margin) {
        status = true; /* 可以投递 */
    } else {
        status = false; /* 无法投递, 但不触发断言 */
    }

    /* 是否为动态事件? */
    if (e->poolId_ != 0U) {
        QF_EVT_REF_CTR_INC_(e); /* 增加引用计数 */
    }

    if (status) { /* 可以投递事件？ */

        --nFree;                  /* 占用一个空闲槽 */
        me->eQueue.nFree = nFree; /* 更新 volatile 变量 */
        if (me->eQueue.nMin > nFree) {
            me->eQueue.nMin = nFree; /* 更新迄今最小空闲槽数 */
        }

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
        QS_TIME_PRE_();                      /* 时间戳 */
        QS_OBJ_PRE_(sender);                 /* 发送者对象 */
        QS_SIG_PRE_(e->sig);                 /* 事件信号 */
        QS_OBJ_PRE_(me);                     /* 接收者 AO 对象 */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
        QS_EQC_PRE_(nFree);                  /* 当前空闲槽数 */
        QS_EQC_PRE_(me->eQueue.nMin);        /* 历史最小空闲槽数 */
        QS_END_NOCRIT_PRE_()

#ifdef Q_UTEST
        /* callback to examine the posted event under the same conditions
         * as producing the #QS_QF_ACTIVE_POST trace record, which are:
         * the local filter for this AO ('me->prio') is set
         */
        if ((QS_priv_.locFilter[me->prio >> 3U] & (1U << (me->prio & 7U))) != 0U) {
            /* callback to examine the posted event */
            QS_onTestPost(sender, me, e, status);
        }
#endif

        /* empty queue? */
        if (me->eQueue.frontEvt == (QEvt *)0) {
            me->eQueue.frontEvt = e;    /* 直接投递事件 */
            QACTIVE_EQUEUE_SIGNAL_(me); /* 发出队列信号 */
        } else {
            /* 队列非空，将事件插入环形缓冲区(FIFO) */
            QF_PTR_AT_(me->eQueue.ring, me->eQueue.head) = e;

            if (me->eQueue.head == 0U) {          /* need to wrap head? */
                me->eQueue.head = me->eQueue.end; /* wrap around */
            }
            --me->eQueue.head; /* advance the head (counter clockwise) */
        }

        QF_CRIT_X_();
    } else { /* 无法投递事件 */

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
        QS_TIME_PRE_();                      /* 时间戳 */
        QS_OBJ_PRE_(sender);                 /* 发送者对象 */
        QS_SIG_PRE_(e->sig);                 /* 事件信号 */
        QS_OBJ_PRE_(me);                     /* 接收者 AO 对象 */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
        QS_EQC_PRE_(nFree);                  /* 当前空闲槽数 */
        QS_EQC_PRE_(margin);                 /* 请求的 margin */
        QS_END_NOCRIT_PRE_()

#ifdef Q_UTEST
        /* callback to examine the posted event under the same conditions
         * as producing the #QS_QF_ACTIVE_POST trace record, which are:
         * the local filter for this AO ('me->prio') is set
         */
        if ((QS_priv_.locFilter[me->prio >> 3U] & (1U << (me->prio & 7U))) != 0U) {
            QS_onTestPost(sender, me, e, status);
        }
#endif

        QF_CRIT_X_();

        QF_gc(e); /* 回收事件, 避免内存泄漏 */
    }

    return status;
}

/****************************************************************************/
/**
 * @brief
 * 将事件投递到活动对象 @p me 的事件队列中, 使用 \b 后进先出(LIFO) 策略。
 *
 * @note
 * LIFO 策略通常只用于自投递事件, 并且需要小心使用, 因为它会改变队列中事件的顺序.
 *
 * @param[in] me 指针
 * @param[in] e  指向要投递到队列的事件的指针
 *
 * @attention
 * 该函数应仅通过宏 QACTIVE_POST_LIFO() 调用.
 */
void QActive_postLIFO_(QActive *const me, QEvt const *const e)
{
    QEvt const *frontEvt; /* 临时变量, 用于避免 volatile 访问的未定义行为 */
    QEQueueCtr nFree;     /* 临时变量, 用于避免 volatile 访问的未定义行为 */
    QF_CRIT_STAT_
    QS_TEST_PROBE_DEF(&QActive_postLIFO_)

    QF_CRIT_E_();
    nFree = me->eQueue.nFree; /* 将 volatile 变量复制到临时变量 */

    /* 测试探针#1: 模拟队列溢出 */
    QS_TEST_PROBE_ID(1,
                     nFree = 0U;)

    /* 队列必须能够接收事件(不能溢出) */
    Q_ASSERT_CRIT_(210, nFree != 0U);

    /* 是否为动态事件? */
    if (e->poolId_ != 0U) {
        QF_EVT_REF_CTR_INC_(e); /* 增加事件的引用计数 */
    }

    --nFree;                  /* 占用一个空闲槽 */
    me->eQueue.nFree = nFree; /* 更新 volatile 变量 */
    if (me->eQueue.nMin > nFree) {
        me->eQueue.nMin = nFree; /* 更新迄今最小空闲槽数 */
    }

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_LIFO, me->prio)
    QS_TIME_PRE_();                      /* 时间戳 */
    QS_SIG_PRE_(e->sig);                 /* 事件信号 */
    QS_OBJ_PRE_(me);                     /* 目标活动对象 */
    QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
    QS_EQC_PRE_(nFree);                  /* 当前空闲槽数 */
    QS_EQC_PRE_(me->eQueue.nMin);        /* 历史最小空闲槽数 */
    QS_END_NOCRIT_PRE_()

#ifdef Q_UTEST
    /* callback to examine the posted event under the same conditions
     * as producing the #QS_QF_ACTIVE_POST trace record, which are:
     * the local filter for this AO ('me->prio') is set
     */
    if ((QS_priv_.locFilter[me->prio >> 3U] & (1U << (me->prio & 7U))) != 0U) {
        QS_onTestPost((QActive *)0, me, e, true);
    }
#endif

    frontEvt            = me->eQueue.frontEvt; /* 将 volatile 读取到临时变量 */
    me->eQueue.frontEvt = e;                   /* 直接将事件放到队列头 */

    /* 队列之前是否为空? */
    if (frontEvt == (QEvt *)0) {
        QACTIVE_EQUEUE_SIGNAL_(me); /* 发出队列信号 */
    } else {
        /* 队列非空, 则将之前的 frontEvt 放回环形缓冲区 */
        ++me->eQueue.tail;
        /* need to wrap the tail? */
        if (me->eQueue.tail == me->eQueue.end) {
            me->eQueue.tail = 0U; /* wrap around */
        }

        QF_PTR_AT_(me->eQueue.ring, me->eQueue.tail) = frontEvt;
    }
    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 该函数的行为取决于 QF 移植所使用的内核/操作系统。
 * 对于内置内核(QV 或 QK), 此函数只能在队列非空时调用, 因此不会阻塞.
 * 对于支持阻塞的内核/OS, 此函数可以阻塞调用线程, 等待事件到达.
 *
 * @param[in,out] me 指针
 *
 * @returns
 * 返回一个指向接收到的事件的指针. 返回的指针保证有效(不可能为 NULL).
 *
 * @note
 * 此函数由 QF 移植内部使用, 用于从活动对象的事件队列中提取事件.
 * 它依赖于事件队列的实现, 有时在 QF 移植中(如 qf_port.h 文件)会被自定义.
 * 根据宏 QACTIVE_EQUEUE_WAIT_() 的定义, 当队列为空时, 此函数可能会阻塞调用线程.
 */
QEvt const *QActive_get_(QActive *const me)
{
    QEQueueCtr nFree;
    QEvt const *e;
    QF_CRIT_STAT_

    QF_CRIT_E_();
    QACTIVE_EQUEUE_WAIT_(me); /* 直接等待事件到达 */

    e                = me->eQueue.frontEvt;   /* 总是从队列前端取出事件 */
    nFree            = me->eQueue.nFree + 1U; /* 复制 volatile 变量到临时 */
    me->eQueue.nFree = nFree;                 /* 更新空闲槽数量 */

    /* 环形缓冲区是否有事件? */
    if (nFree <= me->eQueue.end) {

        /* 从队列尾部取出事件 */
        me->eQueue.frontEvt = QF_PTR_AT_(me->eQueue.ring, me->eQueue.tail);
        if (me->eQueue.tail == 0U) {          /* need to wrap the tail? */
            me->eQueue.tail = me->eQueue.end; /* wrap around */
        }
        --me->eQueue.tail;

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_GET, me->prio)
        QS_TIME_PRE_();                      /* 时间戳 */
        QS_SIG_PRE_(e->sig);                 /* 事件信号 */
        QS_OBJ_PRE_(me);                     /* 活动对象 */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
        QS_EQC_PRE_(nFree);                  /* 空闲槽数量 */
        QS_END_NOCRIT_PRE_()
    } else {
        me->eQueue.frontEvt = (QEvt *)0; /* 队列为空 */

        /* 队列中所有条目必须都是空闲的 (+1 for frontEvt) */
        Q_ASSERT_CRIT_(310, nFree == (me->eQueue.end + 1U));

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_GET_LAST, me->prio)
        QS_TIME_PRE_();                      /* 时间戳 */
        QS_SIG_PRE_(e->sig);                 /* 事件信号 */
        QS_OBJ_PRE_(me);                     /* 活动对象 */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* 池 ID 和引用计数 */
        QS_END_NOCRIT_PRE_()
    }
    QF_CRIT_X_();
    return e;
}

/****************************************************************************/
/**
 * @brief
 * 查询指定优先级 @p prio 的活动对象事件队列自启动以来的最小空闲数.
 *
 * @note
 * 该函数仅在使用原生 QF 事件队列实现时可用.
 * 如果查询未使用的优先级队列, QF 会触发断言.
 * (在调用 QF_add_() 之后, 该优先级才被 QF 视为已使用.)
 *
 * @param[in] prio  要查询队列的活动对象的优先级
 *
 * @returns
 * 自活动对象启动以来, 指定优先级 @p prio 的事件队列中出现过的最小空闲数。
 */
uint_fast16_t QF_getQueueMin(uint_fast8_t const prio)
{
    uint_fast16_t min;
    QF_CRIT_STAT_

    Q_REQUIRE_ID(400, (prio <= QF_MAX_ACTIVE) && (QF_active_[prio] != (QActive *)0));

    QF_CRIT_E_();
    min = (uint_fast16_t)QF_active_[prio]->eQueue.nMin;
    QF_CRIT_X_();

    return min;
}

/****************************************************************************/

#ifdef Q_SPY
static void QTicker_init_(QHsm *const me, void const *par,
                          uint_fast8_t const qs_id);
static void QTicker_dispatch_(QHsm *const me, QEvt const *const e,
                              uint_fast8_t const qs_id);
/*! virtual function to asynchronously post (FIFO) an event to an AO */
static bool QTicker_post_(QActive *const me, QEvt const *const e,
                          uint_fast16_t const margin, void const *const sender);
#else
static void QTicker_init_(QHsm *const me, void const *par);
static void QTicker_dispatch_(QHsm *const me, QEvt const *const e);
static bool QTicker_post_(QActive *const me, QEvt const *const e,
                          uint_fast16_t const margin);
#endif

static void QTicker_postLIFO_(QActive *const me, QEvt const *const e);

/*! 将指针向下转换为 QTicker 指针。 */
/**
 * @brief
 * 该宏封装了向 (QTicker *) 的向下转换, 用于 QTicker_init_() 和 QTicker_dispatch_() 中.
 */
#define QTICKER_CAST_(me_) ((QActive *)(me_))

/*..........................................................................*/
/*! "constructor" of QTicker */
void QTicker_ctor(QTicker *const me, uint_fast8_t tickRate)
{
    static QActiveVtable const vtable = {/* QActive virtual table */
                                         {&QTicker_init_,
                                          &QTicker_dispatch_
#ifdef Q_SPY
                                          ,
                                          &QHsm_getStateHandler_
#endif
                                         },
                                         &QActive_start_,
                                         &QTicker_post_,
                                         &QTicker_postLIFO_};
    QActive_ctor(&me->super, Q_STATE_CAST(0)); /* superclass' ctor */
    me->super.super.vptr = &vtable.super;      /* hook the vptr */

    /* reuse eQueue.head for tick-rate */
    me->super.eQueue.head = (QEQueueCtr)tickRate;
}
/*..........................................................................*/
#ifdef Q_SPY
static void QTicker_init_(QHsm *const me, void const *par,
                          uint_fast8_t const qs_id)
#else
static void QTicker_init_(QHsm *const me, void const *par)
#endif
{
    (void)me;
    (void)par;
#ifdef Q_SPY
    (void)qs_id; /* unused parameter */
#endif
    QTICKER_CAST_(me)->eQueue.tail = 0U;
}
/*..........................................................................*/
#ifdef Q_SPY
static void QTicker_dispatch_(QHsm *const me, QEvt const *const e,
                              uint_fast8_t const qs_id)
#else
static void QTicker_dispatch_(QHsm *const me, QEvt const *const e)
#endif
{
    QEQueueCtr nTicks; /* # ticks since the last call */
    QF_CRIT_STAT_

        (void)
    e; /* unused parameter */
#ifdef Q_SPY
    (void)qs_id; /* unused parameter */
#endif

    QF_CRIT_E_();
    nTicks                         = QTICKER_CAST_(me)->eQueue.tail; /* save the # of ticks */
    QTICKER_CAST_(me)->eQueue.tail = 0U;                             /* clear the # ticks */
    QF_CRIT_X_();

    for (; nTicks > 0U; --nTicks) {
        QF_TICK_X((uint_fast8_t)QTICKER_CAST_(me)->eQueue.head, me);
    }
}
/*..........................................................................*/
#ifndef Q_SPY
static bool QTicker_post_(QActive *const me, QEvt const *const e,
                          uint_fast16_t const margin)
#else
static bool QTicker_post_(QActive *const me, QEvt const *const e,
                          uint_fast16_t const margin,
                          void const *const sender)
#endif
{
    QF_CRIT_STAT_

        (void)
    e;            /* unused parameter */
    (void)margin; /* unused parameter */

    QF_CRIT_E_();
    if (me->eQueue.frontEvt == (QEvt *)0) {

        static QEvt const tickEvt = {0U, 0U, 0U};
        me->eQueue.frontEvt       = &tickEvt; /* deliver event directly */
        --me->eQueue.nFree;                   /* one less free event */

        QACTIVE_EQUEUE_SIGNAL_(me); /* signal the event queue */
    }

    ++me->eQueue.tail; /* account for one more tick event */

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
    QS_TIME_PRE_();      /* timestamp */
    QS_OBJ_PRE_(sender); /* the sender object */
    QS_SIG_PRE_(0U);     /* the signal of the event */
    QS_OBJ_PRE_(me);     /* this active object */
    QS_2U8_PRE_(0U, 0U); /* pool Id & refCtr of the evt */
    QS_EQC_PRE_(0U);     /* number of free entries */
    QS_EQC_PRE_(0U);     /* min number of free entries */
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();

    return true; /* the event is always posted correctly */
}
/*..........................................................................*/
static void QTicker_postLIFO_(QActive *const me, QEvt const *const e)
{
    (void)me; /* unused parameter */
    (void)e;  /* unused parameter */
    Q_ERROR_ID(900);
}
