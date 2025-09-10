/**
 * @file
 * @brief Cooperative QV kernel, definition of QP::QV_readySet_ and
 * implementation of kernel-specific functions.
 * @ingroup qv
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.1
 * Last updated on  2020-09-18
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
#include "qf_pkg.h"  /* QF package-scope internal interface */
#include "qassert.h" /* QP embedded systems-friendly assertions */
#ifdef Q_SPY         /* QS software tracing enabled? */
#include "qs_port.h" /* QS port */
#include "qs_pkg.h"  /* QS facilities for pre-defined trace records */
#else
#include "qs_dummy.h" /* disable the QS software tracing */
#endif                /* Q_SPY */

/* 防止在错误的项目中包含此源文件 */
#ifndef QV_H
#error "Source file included in a project NOT based on the QV kernel"
#endif /* QV_H */

Q_DEFINE_THIS_MODULE("qv")

/* Package-scope objects ****************************************************/
QPSet QV_readySet_; /* QV 活动对象的就绪集合 */

/****************************************************************************/
/**
 * @brief
 * 初始化 QF, 必须在调用其他任何 QF 函数之前且仅调用一次.
 * 通常在 main() 中调用 QF_init(), 甚至在初始化板级支持包 (BSP) 之前.
 *
 * @note QF_init() 会清零内部的 QF 变量，这样即使启动代码没有清零未初始化数据(C 标准要求的),
 * 框架仍然可以正确启动.
 */
void QF_init(void)
{
    QF_maxPool_      = 0U;
    QF_subscrList_   = (QSubscrList *)0;
    QF_maxPubSignal_ = 0;

    QF_bzero(&QF_timeEvtHead_[0], sizeof(QF_timeEvtHead_));
    QF_bzero(&QF_active_[0], sizeof(QF_active_));
    QF_bzero(&QV_readySet_, sizeof(QV_readySet_));

#ifdef QV_INIT
    QV_INIT(); /* port-specific initialization of the QV kernel */
#endif
}

/****************************************************************************/
/**
 * @brief
 * 该函数用于停止 QF 应用程序. 调用此函数后, QF 会尝试优雅地关闭应用程序.
 * 这种优雅关闭可能需要一些时间才能完成. 该函数的典型用途是:
 * - 终止 QF 应用程序以返回操作系统, 或
 * - 处理需要关闭(并可能重置)系统的致命错误.
 *
 * @attention
 * 调用 QF_stop() 后, 应用程序必须终止, 不能继续运行.
 * 特别地, QF_stop() 不应该之后再调用 QF_init() 来"复活"应用程序.
 */
void QF_stop(void)
{
    QF_onCleanup(); /* 应用程序特定的清理回调 */
                    /* 对于协作式 QV 内核无需其他操作 */
}

/****************************************************************************/
/**
 * @brief
 * 通常在 main() 中调用 QF_run(), 在初始化 QF 并至少启动一个活动对象
 * 通过 QACTIVE_START()
 *
 * @returns 在 QV 内核中, QF_run() 函数不会返回.
 */
int_t QF_run(void)
{
#ifdef Q_SPY
    uint_fast8_t pprev = 0U; /* 上一次使用的优先级 */
#endif

    QF_onStartup(); /* 应用程序特定的启动回调 */

    /* QV 内核的组合事件循环和后台循环... */
    QF_INT_DISABLE();

    /* 生成 QS_QF_RUN 跟踪记录 */
    QS_BEGIN_NOCRIT_PRE_(QS_QF_RUN, 0U)
    QS_END_NOCRIT_PRE_()

    for (;;) {
        QEvt const *e;
        QActive *a;
        uint_fast8_t p;

        /* 找出就绪状态中优先级最高的活动对象 */
        if (QPSet_notEmpty(&QV_readySet_)) {
            QPSet_findMax(&QV_readySet_, p);
            a = QF_active_[p];

#ifdef Q_SPY
            QS_BEGIN_NOCRIT_PRE_(QS_SCHED_NEXT, a->prio)
            QS_TIME_PRE_();     /* 时间戳 */
            QS_2U8_PRE_(p,      /* 被调度活动对象的优先级 */
                        pprev); /* 上一次的优先级 */
            QS_END_NOCRIT_PRE_()

            pprev = p; /* 更新上一次优先级 */
#endif                 /* Q_SPY */

            QF_INT_ENABLE();

            /* 执行完成运行(RTC)步骤：
             * 1. 从活动对象的事件队列中取出事件, 此时队列必须非空, Vanilla 内核会断言这一点。
             * 2. 将事件分发到活动对象的状态机。
             * 3. 判断事件是否为垃圾, 如果是则回收。
             */
            e = QActive_get_(a);
            QHSM_DISPATCH(&a->super, e, a->prio);
            QF_gc(e);

            QF_INT_DISABLE();

            if (a->eQueue.frontEvt == (QEvt *)0) { /* 事件队列空? */
                QPSet_remove(&QV_readySet_, p);
            }
        } else { /* 没有就绪的活动对象 --> 空闲 */
#ifdef Q_SPY
            if (pprev != 0U) {
                QS_BEGIN_NOCRIT_PRE_(QS_SCHED_IDLE, 0U)
                QS_TIME_PRE_();    /* 时间戳 */
                QS_U8_PRE_(pprev); /* 上一次的优先级 */
                QS_END_NOCRIT_PRE_()

                pprev = 0U; /* 更新上一次优先级 */
            }
#endif /* Q_SPY */

            /* QV_onIdle() 必须在中断禁止的情况下调用, 因为判断空闲状态
             * (队列中没有事件) 可能随时被中断改变, 中断可能向队列投递事件。
             * QV_onIdle() 必须在内部使能中断, 同时可能将 CPU 置于节能模式.
             */
            QV_onIdle();

            QF_INT_DISABLE();
        }
    }
#ifdef __GNUC__ /* GNU compiler? */
    return 0;
#endif
}

/****************************************************************************/
/**
 * @brief
 * 启动活动对象 (AO) 的执行, 并将 AO 注册到框架中.
 * 同时执行 AO 状态机中的最顶层初始转换.
 * 该初始转换在被调用者的执行线程中完成.
 *
 * @param[in,out] me      指针
 * @param[in]     prio    启动活动对象时的优先级
 * @param[in]     qSto    指向事件队列环形缓冲区的存储指针
 *                        (仅用于内置的 ::QEQueue)
 * @param[in]     qLen    事件队列长度 [事件数]
 * @param[in]     stkSto  栈存储指针 (在 QV 内核中必须为 NULL)
 * @param[in]     stkSize 栈大小 [字节]
 * @param[in]     par     额外参数指针 (可以为 NULL)
 *
 * @note 此函数应通过宏 QACTIVE_START() 调用.
 */
void QActive_start_(QActive *const me, uint_fast8_t prio,
                    QEvt const **const qSto, uint_fast16_t const qLen,
                    void *const stkSto, uint_fast16_t const stkSize,
                    void const *const par)
{
    (void)stkSize; /* unused parameter */

    /** @pre 优先级必须在范围内, 并且不能提供栈存储, 因为 QV 内核不需要每个 AO 的独立栈 */
    Q_REQUIRE_ID(500, (0U < prio) && (prio <= QF_MAX_ACTIVE) && (stkSto == (void *)0));

    QEQueue_init(&me->eQueue, qSto, qLen); /* 初始化内置队列 */
    me->prio = (uint8_t)prio;              /* 设置 AO 当前的优先级 */
    QF_add_(me);                           /*  添加到 QF */

    QHSM_INIT(&me->super, par, me->prio); /* 执行最顶层初始转换 */
    QS_FLUSH();                           /* 将跟踪缓冲区刷新到主机 */
}
