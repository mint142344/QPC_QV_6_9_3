/**
 * @file
 * @brief ::QActive services and @ref qf support code
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.8.2
 * Last updated on  2020-07-17
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

Q_DEFINE_THIS_MODULE("qf_act")

/* public objects ***********************************************************/
QActive *QF_active_[QF_MAX_ACTIVE + 1U]; /* 仅供 QF 移植层使用 */

/****************************************************************************/
/**
 * @brief
 * 将给定的活动对象添加到 QF 框架管理的活动对象列表中.
 * 该函数不应由应用程序直接调用, 仅由 QP 移植层调用.
 *
 * @param[in]  a  指向要添加到框架中的活动对象的指针。
 *
 * @note 在调用此函数之前，应先设置活动对象 @p a 的优先级。
 */
void QF_add_(QActive *const a)
{
    uint_fast8_t p = (uint_fast8_t)a->prio;
    QF_CRIT_STAT_

    /** @pre 活动对象的优先级不得为零, 且不得超过最大值 #QF_MAX_ACTIVE.
     * 同时, 该优先级不得被其他活动对象占用. QF 要求每个活动对象具有唯一的优先级.
     */
    Q_REQUIRE_ID(100, (0U < p) && (p <= QF_MAX_ACTIVE) && (QF_active_[p] == (QActive *)0));

    QF_CRIT_E_();
    QF_active_[p] = a; /* 在该优先级注册活动对象 */
    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 将给定的活动对象从 QF 框架管理的活动对象列表中移除.
 * 该函数不应由应用程序直接调用, 仅由 QP 移植层调用.
 *
 * @param[in]  a  指向要从框架中移除的活动对象的指针.
 *
 * @note
 * 从框架中移除的活动对象将不再参与发布-订阅事件交换.
 */
void QF_remove_(QActive *const a)
{
    uint_fast8_t p = (uint_fast8_t)a->prio;
    QF_CRIT_STAT_

    /** @pre 活动对象的优先级不得为零且不得超过最大值 #QF_MAX_ACTIVE.
     * 同时, 该优先级必须已经在框架中注册.
     */
    Q_REQUIRE_ID(200, (0U < p) && (p <= QF_MAX_ACTIVE) && (QF_active_[p] == a));

    QF_CRIT_E_();
    QF_active_[p]      = (QActive *)0;    /* 释放该优先级 */
    a->super.state.fun = Q_STATE_CAST(0); /* 状态置无效 */
    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 封装指针递增操作的宏
 *
 * @param[in,out]  p_  要递增的指针。
 */
#define QF_PTR_INC_(p_) (++(p_))

/****************************************************************************/
/**
 * @brief
 * 逐字节清零内存缓冲区
 *
 * @param[in]  start  指向内存缓冲区起始地址的指针
 * @param[in]  len    要清零的内存缓冲区长度(字节数)
 *
 * @note 该函数的主要用途是在启动时清零内部 QF 变量.
 * 这样可以避免某些编译器和工具链提供的非标准启动代码没有将未初始化变量置零的问题,
 * 而 ANSI C 标准要求必须清零未初始化变量.
 */
void QF_bzero(void *const start, uint_fast16_t len)
{
    uint8_t *ptr = (uint8_t *)start;
    uint_fast16_t n;
    for (n = len; n > 0U; --n) {
        *ptr = 0U;
        QF_PTR_INC_(ptr);
    }
}

/* log-base-2 implementation ************************************************/
#ifndef QF_LOG2

uint_fast8_t QF_LOG2(QPSetBits x)
{
    static uint8_t const log2LUT[16] = {
        0U, 1U, 2U, 2U, 3U, 3U, 3U, 3U,
        4U, 4U, 4U, 4U, 4U, 4U, 4U, 4U};
    uint_fast8_t n = 0U;
    QPSetBits t;

#if (QF_MAX_ACTIVE > 16)
    t = (QPSetBits)(x >> 16);
    if (t != 0U) {
        n += 16U;
        x = t;
    }
#endif
#if (QF_MAX_ACTIVE > 8)
    t = (x >> 8);
    if (t != 0U) {
        n += 8U;
        x = t;
    }
#endif
    t = (x >> 4);
    if (t != 0U) {
        n += 4U;
        x = t;
    }
    return n + log2LUT[x];
}

#endif /* QF_LOG2 */
