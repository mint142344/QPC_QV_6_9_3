/**
 * @file
 * @brief QP native, platform-independent memory pool ::QMPool interface.
 * @ingroup qf
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
#ifndef QMPOOL_H
#define QMPOOL_H

/****************************************************************************/
#ifndef QF_MPOOL_SIZ_SIZE
/*! 宏, 用于重写默认的 ::QMPoolSize 类型大小 [字节] 可选取值: 1U, 2U 或 4U; 默认值为 2U */
#define QF_MPOOL_SIZ_SIZE 2U
#endif

#if (QF_MPOOL_SIZ_SIZE == 1U)

/*! 用于存储内存块大小的数据类型, 基于宏 \b #QF_MPOOL_SIZ_SIZE 的配置 */
/**
 * @brief
 * 该数据类型的取值范围决定了 原生 QF 事件池 能够管理的单个内存块的最大大小
 */
typedef uint8_t QMPoolSize;
#elif (QF_MPOOL_SIZ_SIZE == 2U)

typedef uint16_t QMPoolSize;
#elif (QF_MPOOL_SIZ_SIZE == 4U)
typedef uint32_t QMPoolSize;
#else
#error "QF_MPOOL_SIZ_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

/****************************************************************************/
#ifndef QF_MPOOL_CTR_SIZE
/*! 宏, 用于重写默认的 ::QMPoolCtr 类型大小 [字节]. 可选取值: 1U, 2U 或 4U; 默认值为 2U */
#define QF_MPOOL_CTR_SIZE 2U
#endif

#if (QF_MPOOL_CTR_SIZE == 1U)

/*! 用于存储内存块计数的数据类型, 基于宏 \b #QF_MPOOL_CTR_SIZE 的配置.  */
/**
 * @brief
 * 该数据类型的取值范围决定了 内存池中可存储的最大块数量.
 */
typedef uint8_t QMPoolCtr;
#elif (QF_MPOOL_CTR_SIZE == 2U)
typedef uint16_t QMPoolCtr;
#elif (QF_MPOOL_CTR_SIZE == 4U)
typedef uint32_t QMPoolCtr;
#else
#error "QF_MPOOL_CTR_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

/****************************************************************************/
/*! 原生 QF 内存池 */
/**
 * @brief
 * 固定块大小 (fixed block-size) 的内存池是一种非常快速且高效的数据结构, 用于动态分配固
 * 定大小的内存块.  内存池能够提供快速且确定性的内存分配与回收, 并且不会产生内存碎片.
 * @n
 * ::QMPool 类描述了 QF 的原生内存池, 它既可以作为 \b 事件池 用于动态事件分配,也可以作为
 * 应用程序中其他对象的快速, 确定性的固定大小堆.
 *
 * @note
 * ::QMPool 仅包含管理内存池所需的数据成员, 并不包含实际的池存储空间.
 * 池的存储空间必须在初始化时由外部提供.
 *
 * @note
 * QF 原生事件池的配置方式是在具体移植的 QF 端口头文件中,
 * 定义宏 \b #QF_EPOOL_TYPE_ 为 ::QMPool.
 */
typedef struct {
    /*! 空闲内存块的链表头指针 */
    void *volatile free_head;

    /*! 池的起始地址 */
    void *start;

    /*! 池中最后一个内存块的地址 */
    void *end;

    /*! 内存块的最大大小(字节) */
    QMPoolSize blockSize;

    /*! 总块数 */
    QMPoolCtr nTot;

    /*! 当前剩余的空闲块数 */
    QMPoolCtr volatile nFree;

    /*! 池中曾经剩余的最小空闲块数(低水位线) */
    /**
     * @brief
     * 该成员记录了池的最低空闲值(low watermark),有助于评估池的大小是否合适
     */
    QMPoolCtr nMin;
} QMPool;

/* 公有函数： */

/*! 初始化原生 QF 内存池 */
void QMPool_init(QMPool *const me, void *const poolSto,
                 uint_fast32_t poolSize, uint_fast16_t blockSize);

/*! 从内存池中获取一个内存块 */
void *QMPool_get(QMPool *const me, uint_fast16_t const margin,
                 uint_fast8_t const qs_id);

/*! 将内存块回收到内存池中 */
void QMPool_put(QMPool *const me, void *b,
                uint_fast8_t const qs_id);

/*! 内存池元素(Memory pool element), 用于为 QMPool 类分配正确对齐的存储空间 */
/**
 * @param[in] evType_ 事件类型(QEvt 的子类名)
 */
#define QF_MPOOL_EL(evType_)                                        \
    struct {                                                        \
        void *sto_[((sizeof(evType_) - 1U) / sizeof(void *)) + 1U]; \
    }

#endif /* QMPOOL_H */
