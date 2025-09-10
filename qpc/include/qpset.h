/**
 * @file
 * @brief QP native, platform-independent priority sets of 32 or 64 elements.
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.8.0
 * Last updated on  2020-01-21
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
#ifndef QPSET_H
#define QPSET_H

#ifndef QF_MAX_ACTIVE
/* 如果没有定义, 则使用默认值 */
#define QF_MAX_ACTIVE 32U
#endif

#if (QF_MAX_ACTIVE < 1U) || (64U < QF_MAX_ACTIVE)
#error "QF_MAX_ACTIVE out of range. Valid range is 1U..64U"
#elif (QF_MAX_ACTIVE <= 8U)
typedef uint8_t QPSetBits;
#elif (QF_MAX_ACTIVE <= 16U)
typedef uint16_t QPSetBits;
#else
/*! QPSet 元素的内部表示位掩码 */
typedef uint32_t QPSetBits;
#endif

#if (QF_MAX_ACTIVE <= 32U)

/****************************************************************************/
/*! 最多支持 32 个元素的优先级集合 */
/**
 * 优先级集合表示一组 \b 已就绪(ready) 且等待调度的活动对象.
 * 调度算法会根据该集合来决定哪个对象获得执行.
 * 该集合最多可以存储 32 个优先级层级.
 */
typedef struct {
    QPSetBits volatile bits; /*!< 位掩码，每一位代表一个元素 */
} QPSet;

/*! 将优先级集合 @p me_ 清空 */
#define QPSet_setEmpty(me_) ((me_)->bits = 0U)

/*! 判断优先级集合 @p me_ 是否为空(返回 TRUE 表示为空) */
#define QPSet_isEmpty(me_) ((me_)->bits == 0U)

/*! 判断优先级集合 @p me_ 是否非空 (返回 TRUE 表示非空) */
#define QPSet_notEmpty(me_) ((me_)->bits != 0U)

/*! 判断优先级集合 @p me_ 是否包含元素 @p n_ */
#define QPSet_hasElement(me_, n_) \
    (((me_)->bits & ((QPSetBits)1 << ((n_) - 1U))) != 0U)

/*! 将元素 @p n_ 插入到集合 @p me_ 中, n_ 的范围是 1U..32U */
#define QPSet_insert(me_, n_) \
    ((me_)->bits |= ((QPSetBits)1 << ((n_) - 1U)))

/*! 将元素 @p n_ 从集合 @p me_ 中移除, n_ 的范围是 1U..32U */
#define QPSet_remove(me_, n_) \
    ((me_)->bits &= (QPSetBits)(~((QPSetBits)1 << ((n_) - 1U))))

/*! 找到集合中的最大元素，并将其赋值给 n_ */
/** @note 如果集合为空, 则 @p n_ 被设置为 0 */
#define QPSet_findMax(me_, n_) \
    ((n_) = QF_LOG2((me_)->bits))

#else /* QF_MAX_ACTIVE > 32U */

/****************************************************************************/
/*! 最多支持 64 个元素的优先级集合 */
/**
 * 优先级集合表示一组已经准备好运行的活动对象, 需要调度算法来考虑它们的执行顺序.
 * 该集合最多可以存储 64 个优先级层级.
 */
typedef struct {
    uint32_t volatile bits[2]; /*!< 两个 32 位位掩码, 分别管理 1..32 和 33..64 的优先级 */
} QPSet;

/*! 将优先级集合 @p me_ 清空 */
#define QPSet_setEmpty(me_)  \
    do {                     \
        (me_)->bits[0] = 0U; \
        (me_)->bits[1] = 0U; \
    } while (false)

/*! 判断优先级集合 @p me_ 是否为空 */
#define QPSet_isEmpty(me_)        \
    (((me_)->bits[0] == 0U)       \
         ? ((me_)->bits[1] == 0U) \
         : false)

/*! 判断优先级集合 @p me_ 是否非空 */
#define QPSet_notEmpty(me_) \
    (((me_)->bits[0] != 0U) \
         ? true             \
         : ((me_)->bits[1] != 0U))

/*! 判断优先级集合 @p me_ 是否包含元素 @p n_ */
#define QPSet_hasElement(me_, n_)                                  \
    (((n_) <= 32U)                                                 \
         ? (((me_)->bits[0] & ((uint32_t)1 << ((n_) - 1U))) != 0U) \
         : (((me_)->bits[1] & ((uint32_t)1 << ((n_) - 33U))) != 0U))

/*! 将元素 @p n_ 插入到集合 @p me_ 中, n_ 的范围是 1..64 */
#define QPSet_insert(me_, n_)                                  \
    do {                                                       \
        if ((n_) <= 32U) {                                     \
            ((me_)->bits[0] |= ((uint32_t)1 << ((n_) - 1U)));  \
        } else {                                               \
            ((me_)->bits[1] |= ((uint32_t)1 << ((n_) - 33U))); \
        }                                                      \
    } while (false)

/*! 将元素 @p n_ 从集合 @p me_ 中移除, n_ 的范围是 1..64 */
#define QPSet_remove(me_, n_)                                               \
    do {                                                                    \
        if ((n_) <= 32U) {                                                  \
            ((me_)->bits[0] &= (uint32_t)(~((uint32_t)1 << ((n_) - 1U))));  \
        } else {                                                            \
            ((me_)->bits[1] &= (uint32_t)(~((uint32_t)1 << ((n_) - 33U)))); \
        }                                                                   \
    } while (false)

/*! 找到集合中的最大元素, 并将其赋值给 @p n_ */
/** @note 如果集合为空, 则 @p n_ 被设置为 0 */
#define QPSet_findMax(me_, n_)                    \
    ((n_) = ((me_)->bits[1] != 0U)                \
                ? (QF_LOG2((me_)->bits[1]) + 32U) \
                : (QF_LOG2((me_)->bits[0])))

#endif /* QF_MAX_ACTIVE */

/****************************************************************************/
/* 基于 2 的对数计算 ...*/
#ifndef QF_LOG2
uint_fast8_t QF_LOG2(QPSetBits x);
#endif /* QF_LOG2 */

#endif /* QPSET_H */
