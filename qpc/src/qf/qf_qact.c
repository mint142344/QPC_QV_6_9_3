/**
 * @file
 * @brief QActive_ctor() definition
 *
 * @description
 * This file must remain separate from the rest to avoid pulling in the
 * "virtual" functions QHsm_init_() and QHsm_dispatch_() in case they
 * are not used by the application.
 *
 * @sa qf_qmact.c
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

/*Q_DEFINE_THIS_MODULE("qf_qact")*/

/****************************************************************************/
/**
 * @brief
 * 本函数执行活动对象初始化的第一步;
 * - 赋值虚函数表指针 (virtual pointer)，
 * - 并调用基类构造函数。
 *
 * @param[in,out] me       指针
 * @param[in]     initial  指向将要分发给 MSM(模型状态机) 的初始事件
 *
 * @note
 * 本函数必须在调用 QMSM_INIT() 之前且仅调用一次
 */
void QActive_ctor(QActive *const me, QStateHandler initial)
{
    static QActiveVtable const vtable = {/* QActive virtual table */
                                         {&QHsm_init_,
                                          &QHsm_dispatch_
#ifdef Q_SPY
                                          ,
                                          &QHsm_getStateHandler_
#endif
                                         },
                                         &QActive_start_,
                                         &QActive_post_,
                                         &QActive_postLIFO_};
    /* 清空整个 QActive 对象, 确保框架能够正确启动,
     * 即使启动代码没有清除未初始化的数据段
     * (这是 C 标准所要求的)。
     */
    QF_bzero(me, sizeof(*me));

    QHsm_ctor(&me->super, initial); /* explicitly call superclass' ctor */
    me->super.vptr = &vtable.super; /* hook the vptr to QActive vtable */
}
