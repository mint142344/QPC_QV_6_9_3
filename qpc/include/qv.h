/**
 * @file
 * @brief QV/C (cooperative "Vanilla" kernel) platform-independent
 * public interface
 * @ingroup qv
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.1
 * Last updated on  2020-09-08
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
#ifndef QV_H
#define QV_H

#include "qequeue.h" /* QV kernel uses the native QP event queue  */
#include "qmpool.h"  /* QV kernel uses the native QP memory pool  */
#include "qpset.h"   /* QV kernel uses the native QP priority set */

/* QV event-queue used for AOs */
#define QF_EQUEUE_TYPE QEQueue

/*! QV idle callback (customized in BSPs) */
/**
 * @brief
 * 当调度器检测到没有事件可供活动对象处理 (即空闲状态) 时,
 * 协作式 QV 内核 (从 QF_run() 调用) 会调用 QV_onIdle().
 * 该回调给应用程序提供了机会, 可以进入省电模式, 或者执行
 * 其他空闲处理 (例如 QS 软件跟踪输出).
 *
 * @note QV_onIdle() 被调用时中断是 \b 禁用的,
 * 因为空闲状态可能随时被中断异步改变.
 * QV_onIdle() 必须在内部启用中断，但不能在进入低功耗模式之前启用.
 * (理想情况下，启用中断和进入低功耗模式应当是原子操作).
 * 至少，该函数必须启用中断，否则中断将永久保持禁用状态.
 *
 * @note QV_onIdle() 仅在原生 (裸机) QF 移植层的协作式 QV 内核中使用，
 * 在其他 QF 移植层中不会使用.
 * 当 QF 与抢占式 QK 结合时, QK 的空闲循环调用不同的函数 QK_onIdle(),
 * 其语义不同于 QV_onIdle().
 * 当 QF 与第三方 RTOS 或内核结合时, 则使用 RTOS 或内核的空闲处理机制,
 * 而不是 QV_onIdle().
 */
void QV_onIdle(void);

/****************************************************************************/
/* 仅供 QP 内部实现使用的接口，应用层代码不会用到 */
#ifdef QP_IMPL

/* QV 内核特有的调度器加锁机制 (但在 QV 中不需要) */
#define QF_SCHED_STAT_
#define QF_SCHED_LOCK_(dummy)   ((void)0)
#define QF_SCHED_UNLOCK_(dummy) ((void)0)

/* QF 原生事件队列操作 */
#define QACTIVE_EQUEUE_WAIT_(me_) \
    Q_ASSERT_ID(0, (me_)->eQueue.frontEvt != (QEvt *)0)

#define QACTIVE_EQUEUE_SIGNAL_(me_) \
    QPSet_insert(&QV_readySet_, (uint_fast8_t)(me_)->prio)

/* QF 原生事件池操作 */
#define QF_EPOOL_TYPE_ QMPool
#define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
    (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))
#define QF_EPOOL_EVENT_SIZE_(p_) ((uint_fast16_t)(p_).blockSize)
#define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
    ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qs_id_)))
#define QF_EPOOL_PUT_(p_, e_, qs_id_) \
    (QMPool_put(&(p_), (e_), (qs_id_)))

extern QPSet QV_readySet_; /*!< QV ready-set of AOs */

#endif /* QP_IMPL */

#endif /* QV_H */
