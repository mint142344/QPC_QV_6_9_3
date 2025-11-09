/**
 * @file
 * @brief QV/C port to ARM Cortex-M, ARM-KEIL toolset
 * @cond
 ******************************************************************************
 * Last updated for version 6.8.0
 * Last updated on  2020-01-25
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
 * along with this program. If not, see <www.gnu.org/licenses/>.
 *
 * Contact information:
 * <www.state-machine.com/licensing>
 * <info@state-machine.com>
 ******************************************************************************
 * @endcond
 */
/* 这个 QV 移植文件是 QP 内部实现的一部分 */
#define QP_IMPL 1U
#include "qf_port.h"

#if (__TARGET_ARCH_THUMB == 3) /* Cortex-M0/M0+/M1(v6-M, v6S-M)? */

/* Cortex-M0/M0+/M1(v6-M, v6S-M) 的汇编手工优化快速 LOG2 函数 */
// clang-format off
__asm uint_fast8_t QF_qlog2(uint32_t x) {
    MOVS    r1,#0

#if (QF_MAX_ACTIVE > 16)
    LSRS    r2,r0,#16
    BEQ.N   QF_qlog2_1
    MOVS    r1,#16
    MOVS    r0,r2
QF_qlog2_1
#endif
#if (QF_MAX_ACTIVE > 8)
    LSRS    r2,r0,#8
    BEQ.N   QF_qlog2_2
    ADDS    r1,r1,#8
    MOVS    r0,r2
QF_qlog2_2
#endif
    LSRS    r2,r0,#4
    BEQ.N   QF_qlog2_3
    ADDS    r1,r1,#4
    MOVS    r0,r2
QF_qlog2_3
    LDR     r2,=QF_qlog2_LUT
    LDRB    r0,[r2,r0]
    ADDS    r0,r1,r0
    BX      lr

    ALIGN

QF_qlog2_LUT
    DCB     0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
}
// clang-format on

#else /* NOT Cortex-M0/M0+/M1(v6-M, v6S-M)? */

#define SCnSCB_ICTR ((uint32_t volatile *)0xE000E004)
#define SCB_SYSPRI  ((uint32_t volatile *)0xE000ED14)
#define NVIC_IP     ((uint32_t volatile *)0xE000E400)

/*
 * 初始化异常优先级和 IRQ 优先级为安全值.
 *
 * 说明:
 * 对于 Cortex-M3/M4/M7, 这个 QV 移植使用 BASEPRI 寄存器来禁止中断.
 * 但是 BASEPRI 不能屏蔽优先级为 0 的中断, 而默认所有中断复位后优先级为 0.
 * 以下代码将 SysTick 和所有 IRQ 优先级设置为 QF_BASEPRI,
 * 这样 QF 的临界区(critical section)可以有效屏蔽它们, 这样可以避免在应
 * 用程序忘记设置“QF aware”中断优先级时破坏 QF 临界区.
 *
 * 应用程序可以在之后修改 QV_init() 设置的中断优先级.
 */
void QV_init(void)
{
    uint32_t n;

    /* 将异常优先级设置为 QF_BASEPRI...
     * SCB_SYSPRI1: 使用故障、总线故障、内存管理故障
     */
    SCB_SYSPRI[1] |= (QF_BASEPRI << 16) | (QF_BASEPRI << 8) | QF_BASEPRI;

    /* SCB_SYSPRI2: SVCall */
    SCB_SYSPRI[2] |= (QF_BASEPRI << 24);

    /* SCB_SYSPRI3:  SysTick, PendSV, Debug */
    SCB_SYSPRI[3] |= (QF_BASEPRI << 24) | (QF_BASEPRI << 16) | QF_BASEPRI;

    // 从 SCnSCB_ICTR 寄存器读取已实现的 IRQ 数量
    n = 8U + ((*SCnSCB_ICTR & 0x7U) << 3); /* (# NVIC_PRIO registers)/4 */
    /* set all implemented IRQ priories to QF_BASEPRI... */
    do {
        --n;
        NVIC_IP[n] = (QF_BASEPRI << 24) | (QF_BASEPRI << 16) | (QF_BASEPRI << 8) | QF_BASEPRI;
    } while (n != 0);
}

#endif /* NOT Cortex-M0/M0+/M1(v6-M, v6S-M)? */
