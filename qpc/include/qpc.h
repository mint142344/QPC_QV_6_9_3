/**
 * @file
 * @brief QP/C public interface including backwards-compatibility layer
 * @ingroup qep qf qv qk qxk qs
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.1
 * Last updated on  2020-09-30
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
#ifndef QPC_H
#define QPC_H

/**
 * @brief 所有使用 QP/C 的应用模块 (*.c 文件) 必须直接或间接包含此头文件.
 */

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
#include "qf_port.h" /* 来自移植目录的 QF/C 移植层 */
#include "qassert.h" /* QP 嵌入式友好的断言库 */

#ifdef Q_SPY         /* 是否启用软件跟踪？ */
#include "qs_port.h" /* 来自移植目录的 QS/C 移植层 */
#else
#include "qs_dummy.h" /* QS/C 空接口 (未启用) */
#endif

/****************************************************************************/
#ifndef QP_API_VERSION

/*! 指定与 QP/C API 版本的向后兼容性. */
/**
 * @brief
 * 例如, QP_API_VERSION=580 会生成与 QP/C 5.8.0 及更新版本兼容的层, 但不兼容 5.8.0 之前的版本.
 * QP_API_VERSION=0 会生成当前支持的最大向后兼容性(默认值).
 * 相反, QP_API_VERSION=9999 表示不生成任何兼容层, 这对于检查应用是否符合最新 QP/C API 非常有用.
 */
#define QP_API_VERSION 0

#endif /* #ifndef QP_API_VERSION */

/* QP API 兼容性层... */

/****************************************************************************/
#if (QP_API_VERSION < 691)

/*! @deprecated enable the QS global filter */
#define QS_FILTER_ON(rec_) QS_GLB_FILTER((rec_))

/*! @deprecated disable the QS global filter */
#define QS_FILTER_OFF(rec_) QS_GLB_FILTER(-(rec_))

/*! @deprecated enable the QS local filter for SM (state machine) object */
#define QS_FILTER_SM_OBJ(obj_) ((void)0)

/*! @deprecated enable the QS local filter for AO (active objects) */
#define QS_FILTER_AO_OBJ(obj_) ((void)0)

/*! @deprecated enable the QS local filter for MP (memory pool) object */
#define QS_FILTER_MP_OBJ(obj_) ((void)0)

/*! @deprecated enable the QS local filter for EQ (event queue) object */
#define QS_FILTER_EQ_OBJ(obj_) ((void)0)

/*! @deprecated enable the QS local filter for TE (time event) object */
#define QS_FILTER_TE_OBJ(obj_) ((void)0)

#ifdef Q_SPY

/*! @deprecated local Filter for a generic application object @p obj_. */
#define QS_FILTER_AP_OBJ(obj_) (QS_priv_.locFilter_AP = (obj_))

/*! @deprecated begin of a user QS record, instead use QS_BEGIN_ID() */
#define QS_BEGIN(rec_, obj_)                                                                                                                                                             \
    if (((QS_priv_.glbFilter[(uint_fast8_t)(rec_) >> 3U] & (1U << ((uint_fast8_t)(rec_) & 7U))) != 0U) && ((QS_priv_.locFilter_AP == (void *)0) || (QS_priv_.locFilter_AP == (obj_)))) { \
        QS_CRIT_STAT_                                                                                                                                                                    \
        QS_CRIT_E_();                                                                                                                                                                    \
        QS_beginRec_((uint_fast8_t)(rec_));                                                                                                                                              \
        QS_TIME_PRE_();                                                                                                                                                                  \
        {

/*! @deprecated Output formatted uint32_t to the QS record */
#define QS_U32_HEX(width_, data_) \
    (QS_u32_fmt_((uint8_t)(((width_) << 4)) | (uint8_t)0x0FU, (data_)))

#else

#define QS_FILTER_AP_OBJ(obj_)    ((void)0)
#define QS_BEGIN(rec_, obj_)      if (false) {
#define QS_U32_HEX(width_, data_) ((void)0)

#endif

/****************************************************************************/
#if (QP_API_VERSION < 660)

/*! @deprecated 转换为 QXThreadHandler 时使用此宏, 建议使用新的 QXThreadHandler 函数签名, 并不要使用类型转换 */
#define Q_XTHREAD_CAST(handler_) ((QXThreadHandler)(handler_))

/****************************************************************************/
#if (QP_API_VERSION < 580)

/*! @deprecated 调用 QMSM_INIT(), 建议使用 QHSM_INIT() */
#define QMSM_INIT(me_, e_) QHSM_INIT((me_), (e_))

/*! @deprecated 调用 QMSM_DISPATCH(), 建议使用 QHSM_DISPATCH() */
#define QMSM_DISPATCH(me_, e_) QHSM_DISPATCH((me_), (e_), 0U)

/****************************************************************************/
#if (QP_API_VERSION < 540)

/*! @deprecated QFsm state machine;
 * 建议使用 ::QHsm. 以 "QFsm 风格" 编写的旧状态机仍然可用,
 * 但内部会使用 ::QHsm 实现.
 * 使用 "QFsm 风格" 已不再有性能优势.
 *
 * @note * 为了效率, 推荐的迁移路径是使用 ::QMsm 状态机和 QM 建模工具。
 */
typedef QHsm QFsm;

/*! @deprecated 状态机构造函数, 建议使用 QHsm_ctor() */
#define QFsm_ctor QHsm_ctor

/*! @deprecated 在 QFsm 状态处理器中调用的宏, 用于忽略(不处理)事件建议使用 Q_SUPER() 代替 */
#define Q_IGNORED() (Q_SUPER(&QHsm_top))

/*! @deprecated 协作式 "Vanilla" 内核的宏，建议使用 QV_onIdle() 代替 */
#define QF_onIdle QV_onIdle

#endif /* QP_API_VERSION < 540 */
#endif /* QP_API_VERSION < 580 */
#endif /* QP_API_VERSION < 660 */
#endif /* QP_API_VERSION < 691 */

#ifdef __cplusplus
}
#endif

#endif /* QPC_H */
