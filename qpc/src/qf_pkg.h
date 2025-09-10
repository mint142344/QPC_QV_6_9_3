/**
 * @file
 * @brief Internal (package scope) QF/C interface.
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.1
 * Last updated on  2020-09-16
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
#ifndef QF_PKG_H
#define QF_PKG_H

/****************************************************************************/
/* QF-specific critical section */
#ifndef QF_CRIT_STAT_TYPE
/*! 这是一个内部宏, 用于定义临界区状态类型 */
/**
 * @brief
 * 该宏的目的是使代码在定义了临界区状态类型和未定义时都能统一书写.
 * 如果宏 #QF_CRIT_STAT_TYPE 被定义, 该内部宏会提供临界区状态变量的定义.
 * 否则, 该宏为空.
 */
#define QF_CRIT_STAT_

/*! 这是一个内部宏, 用于进入临界区 */
/**
 * @brief
 * 该宏的目的是使代码在定义了临界区状态类型和未定义时都能统一书写.
 * 如果宏 #QF_CRIT_STAT_TYPE 被定义, 该内部宏会调用 QF_CRIT_ENTRY() 并传入状态变量作为参数.
 * 否则, QF_CRIT_ENTRY() 使用一个占位参数(dummy)调用.
 */
#define QF_CRIT_E_() QF_CRIT_ENTRY(dummy)

/*! 这是一个内部宏, 用于退出临界区 */
/**
 * @brief
 * 该宏的目的是使代码在定义了临界区状态类型和未定义时都能统一书写.
 * 如果宏 #QF_CRIT_STAT_TYPE 被定义, 该内部宏会调用 #QF_CRIT_EXIT 并传入状态变量作为参数.
 * 否则, #QF_CRIT_EXIT 使用一个占位参数(dummy)调用.
 */
#define QF_CRIT_X_() QF_CRIT_EXIT(dummy)

#elif (!defined QF_CRIT_STAT_)
#define QF_CRIT_STAT_ QF_CRIT_STAT_TYPE critStat_;
#define QF_CRIT_E_()  QF_CRIT_ENTRY(critStat_)
#define QF_CRIT_X_()  QF_CRIT_EXIT(critStat_)
#endif

/****************************************************************************/
/* 临界区内部断言 */
#ifdef Q_NASSERT /* Q_NASSERT defined--assertion checking disabled */

#define Q_ASSERT_CRIT_(id_, test_)  ((void)0)
#define Q_REQUIRE_CRIT_(id_, test_) ((void)0)
#define Q_ERROR_CRIT_(id_)          ((void)0)

#else /* Q_NASSERT not defined--assertion checking enabled */

#define Q_ASSERT_CRIT_(id_, test_)                        \
    do {                                                  \
        if ((test_)) {                                    \
        } else {                                          \
            QF_CRIT_X_();                                 \
            Q_onAssert(&Q_this_module_[0], (int_t)(id_)); \
        }                                                 \
    } while (false)

#define Q_REQUIRE_CRIT_(id_, test_) Q_ASSERT_CRIT_((id_), (test_))

#define Q_ERROR_CRIT_(id_)                            \
    do {                                              \
        QF_CRIT_X_();                                 \
        Q_onAssert(&Q_this_module_[0], (int_t)(id_)); \
    } while (false)

#endif /* Q_NASSERT */

/****************************************************************************/
/*! 内部实现(应仅通过虚表使用)*/

/*! 活动对象启动实现 */
void QActive_start_(QActive *const me, uint_fast8_t prio,
                    QEvt const **const qSto, uint_fast16_t const qLen,
                    void *const stkSto, uint_fast16_t const stkSize,
                    void const *const par);

/*! 从活动对象的事件队列中获取一个事件 */
QEvt const *QActive_get_(QActive *const me);

#ifdef Q_SPY
/*! 活动对象的事件投递(FIFO)操作实现 */
bool QActive_post_(QActive *const me, QEvt const *const e,
                   uint_fast16_t const margin,
                   void const *const sender);
#else
bool QActive_post_(QActive *const me, QEvt const *const e,
                   uint_fast16_t const margin);
#endif

/*! 活动对象的事件投递(LIFO)操作实现 */
void QActive_postLIFO_(QActive *const me, QEvt const *const e);

/****************************************************************************/
/*! 每个时钟节拍速率对应的时间事件链表头 */
extern QTimeEvt QF_timeEvtHead_[QF_MAX_TICK_RATE];

/** 以下标志和位掩码用于 QTimeEvt(继承自 QEvt)中 refCtr_ 属性.
 * 该属性不用于时间事件的引用计数, 因为 @c poolId_ 属性为 0(“静态事件”).
 */
#define TE_IS_LINKED    (1U << 7)
#define TE_WAS_DISARMED (1U << 6)
#define TE_TICK_RATE    0x0FU

extern QF_EPOOL_TYPE_ QF_pool_[QF_MAX_EPOOL]; /*!< 分配事件池 */
extern uint_fast8_t QF_maxPool_;              /*!< 已初始化的事件池数量 */
extern QSubscrList *QF_subscrList_;           /*!< 订阅者列表数组 */
extern enum_t QF_maxPubSignal_;               /*!< 最大已发布信号 */

/*! 表示 Native QF 内存池中一个空闲块的结构体 */
typedef struct QFreeBlock {
    struct QFreeBlock *volatile next;
} QFreeBlock;

/* 内部辅助宏 ***************************************************/

/*! 去掉事件指针 @p e_ 的 const */
#define QF_EVT_CONST_CAST_(e_) ((QEvt *)(e_))

/*! 增加事件 @p e_ 的 refCtr (去掉 const 属性后) */
#define QF_EVT_REF_CTR_INC_(e_) (++QF_EVT_CONST_CAST_(e_)->refCtr_)

/*! 减少事件 @p e_ 的 refCtr(去掉 const 属性后) */
#define QF_EVT_REF_CTR_DEC_(e_) (--QF_EVT_CONST_CAST_(e_)->refCtr_)

/*! 从基指针 @p base_ 访问索引 @p i_ 的元素 */
#define QF_PTR_AT_(base_, i_) ((base_)[(i_)])

/**
 * @brief
 * 该宏专门用于检查返回到池中的块指针的范围.
 */
#define QF_PTR_RANGE_(x_, min_, max_) (((min_) <= (x_)) && ((x_) <= (max_)))

#endif /* QF_PKG_H */
