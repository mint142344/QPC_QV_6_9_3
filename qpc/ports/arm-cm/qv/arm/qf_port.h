/**
 * @file
 * @brief QF/C port to Cortex-M, cooperative QV kernel, ARM-KEIL toolset
 * @cond
 ******************************************************************************
 * Last updated for version 6.3.8
 * Last updated on  2019-01-10
 *
 *                    Q u a n t u m  L e a P s
 *                    ------------------------
 *                    Modern Embedded Software
 *
 * Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
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
 * along with this program. If not, see <www.gnu.org/licenses/>.
 *
 * Contact information:
 * <www.state-machine.com/licensing>
 * <info@state-machine.com>
 ******************************************************************************
 * @endcond
 */
#ifndef QF_PORT_H
#define QF_PORT_H

/* 系统时钟滴答率的最大数量  */
#define QF_MAX_TICK_RATE 2U

/* QF 中断禁止/允许和 log2() 函数... */
#if (__TARGET_ARCH_THUMB == 3) /* Cortex-M0/M0+/M1(v6-M, v6S-M)? */

/* 应用程序中活跃对象的最大数量, 见 NOTE1 */
#define QF_MAX_ACTIVE 8

/* Cortex-M0/M0+/M1(v6-M, v6S-M) 中断禁止策略, 见 NOTE2 */
#define QF_INT_DISABLE() __disable_irq()
#define QF_INT_ENABLE()  __enable_irq()

/* QF 临界区进入/退出(无条件禁止中断) */
/*#define QF_CRIT_STAT_TYPE 未定义 */
#define QF_CRIT_ENTRY(dummy) QF_INT_DISABLE()
#define QF_CRIT_EXIT(dummy)  QF_INT_ENABLE()

/* CMSIS 中的"QF-aware"中断优先级阈值, 见 NOTE2 和 NOTE4 */
#define QF_AWARE_ISR_CMSIS_PRI 0

/* Cortex-M0/M0+/M1(v6-M, v6S-M) 手工优化的 LOG2 汇编实现 */
#define QF_LOG2(n_) QF_qlog2((uint32_t)(n_))

#else /* Cortex-M3/M4/M7 */

/* 应用程序中活跃对象的最大数量, 见 NOTE1 */
#define QF_MAX_ACTIVE        16U

/* Cortex-M3/M4/M7 中 PRIMASK 禁止中断的替代方法 */
#define QF_PRIMASK_DISABLE() __disable_irq()
#define QF_PRIMASK_ENABLE()  __enable_irq()

/* Cortex-M3/M4/M7 中断禁止策略, 见 NOTE3 和 NOTE4 */
#define QF_INT_DISABLE()            \
    do {                            \
        QF_PRIMASK_DISABLE();       \
        QF_set_BASEPRI(QF_BASEPRI); \
        QF_PRIMASK_ENABLE();        \
    } while (false)
#define QF_INT_ENABLE()        QF_set_BASEPRI(0U)

/* QF 临界区进入/退出(无条件禁止中断) */
/*#define QF_CRIT_STAT_TYPE 未定义 */
#define QF_CRIT_ENTRY(dummy)   QF_INT_DISABLE()
#define QF_CRIT_EXIT(dummy)    QF_INT_ENABLE()

/* BASEPRI 阈值, 用于"QF-aware"中断, 见 NOTE3 */
#define QF_BASEPRI             0x3F

/* CMSIS 中的"QF-aware"中断优先级阈值, 见 NOTE5 */
#define QF_AWARE_ISR_CMSIS_PRI (QF_BASEPRI >> (8 - __NVIC_PRIO_BITS))

/* Cortex-M3/M4/M7 提供 CLZ 指令用于快速 LOG2 */
#define QF_LOG2(n_)            ((uint_fast8_t)(32U - __clz((unsigned)(n_))))

/* 获取 BASEPRI 寄存器的内联函数 */
static __inline unsigned QF_get_BASEPRI(void)
{
    register unsigned volatile __regBasePri __asm("basepri");
    return __regBasePri;
}

/* 设置 BASEPRI 寄存器的内联函数 */
static __inline void QF_set_BASEPRI(unsigned basePri)
{
    register unsigned volatile __regBasePri __asm("basepri");
    __regBasePri = basePri;
}

#endif

#define QF_CRIT_EXIT_NOP() __asm("isb")

#include "qep_port.h" /* QEP 移植层 */

#if (__TARGET_ARCH_THUMB == 3) /* Cortex-M0/M0+/M1(v6-M, v6S-M)? */
/* 手工优化的快速 LOG2 汇编实现 */
uint_fast8_t QF_qlog2(uint32_t x);
#endif /* Cortex-M0/M0+/M1(v6-M, v6S-M) */

#include "qv_port.h" /* QV 协作式内核移植层 */
#include "qf.h"      /* QF 平台无关公共接口 */

/*****************************************************************************
 * \b NOTE1:
 * QF_MAX_ACTIVE 表示活跃对象的最大数量, 必要时可增加到 64. 这里设置为较小值以节省 RAM
 *
 * \b NOTE2:
 * 在 Cortex-M0/M0+/M1 (v6-M, v6S-M 架构) 中, 中断禁止策略使用 PRIMASK 寄存器全局禁止中断.
 * QF_AWARE_ISR_CMSIS_PRI 设置为 0, 表示所有中断都是“QF-aware”
 *
 * \b NOTE3:
 * 在 Cortex-M3/M4/M7 中, 中断禁止策略使用 BASEPRI 寄存器 (Cortex-M0/M0+/M1 不支持) 禁止
 * 低于 QF_BASEPRI 阈值的中断. 优先级高于 QF_BASEPRI 的中断 (数值小于 QF_BASEPRI) 不会被
 * 禁止. 这些自由运行中断延迟非常低, \b 但它们不能调用任何 QF 服务, 因为 QF 对它们"未知"
 * (称为"QF-unaware 中断"). 因此, 只有优先级数值等于或高于 QF_BASEPRI 的中断
 * (称为"QF-aware 中断") 才可以调用 QF 服务.
 *
 * \b NOTE4:
 * 宏 \b QF_AWARE_ISR_CMSIS_PRI 在应用程序中用于作为枚举"QF-aware"中断优先级的偏移量.
 * "QF-aware"中断的数值优先级必须大于或等于 QF_AWARE_ISR_CMSIS_PRI.
 * 基于 QF_AWARE_ISR_CMSIS_PRI 的数值可以直接传递给 CMSIS 函数 NVIC_SetPriority(),
 * 该函数会将其根据 (8 - __NVIC_PRIO_BITS) 移位到正确的位位置.
 * 其中 \b __NVIC_PRIO_BITS 是 CMSIS 宏, 定义了 NVIC 中实现的优先级位数.
 * 请注意, 宏 QF_AWARE_ISR_CMSIS_PRI 仅供应用程序使用, 不在 QF 移植层内部使用,
 * 因此 QF 移植层保持通用性, 不依赖 NVIC 实际实现的优先级位数.
 *
 * \b NOTE5:
 * 使用 BASEPRI 寄存器选择性禁止"QF-aware"中断, 在 ARM Cortex-M7 core r0p1 核心上存在
 * 问题(参见 ARM-EPM-064408，勘误 837070).  ARM 推荐的解决方法是, 在访问 BASEPRI 寄存器
 * 的 MSR 指令前后加入 CPSID i / CPSIE i 指令对, 这在宏 QF_INT_DISABLE() 中已经实现.
 * 该解决方法同样适用于 Cortex-M3/M4 核心.
 */

#endif /* QF_PORT_H */
