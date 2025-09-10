/**
 * @file
 * @brief QP natvie, platform-independent, thread-safe event queue interface
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.8.0
 * Last updated on  2020-01-18
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
#ifndef QEQUEUE_H
#define QEQUEUE_H

/**
 * @brief
 * 这个头文件必须被包含在所有使用 QF 原生事件队列 (native QF event queue) 的端口中.
 * 另外, 当应用程序使用 QActive_defer()/QActive_recall() 时, 该文件也需要包含到 QP/C 库中.
 * 最后, 当"原始的 (raw)"线程安全队列用于活动对象与非框架实体 (比如 ISR, 中断服务程序,
 * 设备驱动程序或遗留代码) 之间通信时, 该文件同样需要包含.
 */

#ifndef QF_EQUEUE_CTR_SIZE

/*! 原生 QF 事件队列中用于 环形缓冲区计数器 的大小 (字节) 可选值：1U, 2U 或 4U; 默认 1U */
/**
 * @brief
 * 这个宏可以在 QF 的移植文件 (qf_port.h) 中定义,
 * 用于配置 ::QEQueueCtr 的类型.
 * 这里如果不定义，默认选择 1 字节。
 */
#define QF_EQUEUE_CTR_SIZE 1U
#endif
#if (QF_EQUEUE_CTR_SIZE == 1U)

/*! 基于宏 \b #QF_EQUEUE_CTR_SIZE 的环形缓冲区计数器数据类型 */
/**
 * @brief
 * 该数据类型的取值范围决定了原生 QF 事件队列所能管理的环形缓冲区的最大长度.
 */
typedef uint8_t QEQueueCtr;
#elif (QF_EQUEUE_CTR_SIZE == 2U)
typedef uint16_t QEQueueCtr;
#elif (QF_EQUEUE_CTR_SIZE == 4U)
typedef uint32_t QEQueueCtr;
#else
#error "QF_EQUEUE_CTR_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

/****************************************************************************/
/*! 原生 QF 事件队列 */
/**
 * @brief
 * 该类描述了原生 QF 事件队列, 它既可以作为活动对象 (Active Object) 的事件队列,
 * 也可以作为一个简单的"原始"事件队列, 用于非框架实体之间的线程安全事件传递,
 * 比如中断服务程序(ISR), 设备驱动程序或第三方组件
 * @n
 * 原生 QF 事件队列的配置方式是在特定 QF 移植文件头(qf_port.h)中定义宏
 * \b #QF_EQUEUE_TYPE 为 ::QEQueue
 * @n
 * ::QEQueue 结构体仅包含用于管理事件队列的数据成员, 但不包含队列缓冲区的存储空间,
 * 缓冲区必须在队列初始化时由外部提供.
 * @n
 * 事件队列仅能存储事件指针, 而不是整个事件对象. 内部实现采用标准的环形缓冲区,
 * 外加一个独立的额外位置, 用于优化最常见的空队列操作性能.
 * @n
 * ::QEQueue 结构体可配合两组函数使用:
 * 第一组函数用于 \b 活动对象的事件队列, 这种情况下队列可能需要在为空时阻塞活动对象任务,
 * 并在有事件加入时解除阻塞. 相关接口包括：
 * QActive_post(), QActive_postLIFO(), QActive_get_(),
 * 此外 QEQueue_init() 用于初始化队列.
 * @n
 * 第二组函数将 ::QEQueue 用作一个 \b 简单的"原始"事件队列, 用于非活动对象的实体之间
 * 传递事件, 例如 ISR. 原始事件队列不能在 get() 操作上阻塞, 但仍是线程安全的,
 * 因为它使用了 QF 临界区保护队列完整性. 相关接口包括:
 * QEQueue_post(), QEQueue_postLIFO(), QEQueue_get(),
 * 此外 QEQueue_init() 用于初始化队列.
 *
 * @note 大多数事件队列操作(无论是活动对象队列还是原始队列)内部都会使用 QF 临界区.
 * 如果底层不支持临界区嵌套, 应注意不要在其他临界区中调用这些操作.
 */
typedef struct QEQueue {
    /*! 指向队列头部事件的指针 */
    /**
     * @brief
     * 所有进入和取出的事件都会先经过 frontEvt 位置。
     * 当队列为空时(这是最常见的情况), frontEvt 位置可以绕过环形缓冲区，
     * 大大优化了队列的性能. 只有事件突发时才会使用环形缓冲区.
     *
     * @note 该属性的另一个作用是指示队列是否为空. 当 frontEvt 为 NULL 时, 表示队列为空.
     */
    QEvt const *volatile frontEvt;

    /*! 指向环形缓冲区起始位置的指针 */
    QEvt const **ring;

    /*! 环形缓冲区尾部相对于起始位置的偏移量 */
    QEQueueCtr end;

    /*! 下一个事件将被插入到缓冲区中的偏移位置 */
    QEQueueCtr volatile head;

    /*! 下一个事件将从缓冲区取出的偏移位置 */
    QEQueueCtr volatile tail;

    /*! 环形缓冲区中剩余的可用事件槽数量 */
    QEQueueCtr volatile nFree;

    /*! 环形缓冲区中历史最小剩余可用事件槽数量 */
    /**
     * @brief
     * 该属性记录了环形缓冲区的最低水位线,
     * 可为事件队列容量规划提供有价值的信息.
     * @sa QF_getQueueMargin()
     */
    QEQueueCtr nMin;
} QEQueue;

/* public class operations */

/*! 初始化原生 QF 事件队列 */
void QEQueue_init(QEQueue *const me,
                  QEvt const **const qSto, uint_fast16_t const qLen);

/*! 向"原始"线程安全事件队列中投递一个事件 (FIFO 方式)*/
bool QEQueue_post(QEQueue *const me, QEvt const *const e,
                  uint_fast16_t const margin, uint_fast8_t const qs_id);

/*! 向"原始"线程安全事件队列中投递一个事件(LIFO 方式). */
void QEQueue_postLIFO(QEQueue *const me, QEvt const *const e,
                      uint_fast8_t const qs_id);

/*! 从"原始"线程安全事件队列中取一个事件. */
QEvt const *QEQueue_get(QEQueue *const me, uint_fast8_t const qs_id);

/*!
 * "原始"线程安全 QF 事件队列操作: 获取队列中当前仍然可用的空闲槽数量.
 */
/**
 * @brief
 * 使用该操作时需要小心, 因为空闲槽数量可能会意外变化.
 * 主要的使用场景是结合事件延迟(event deferral).
 * 在这种情况下, 队列仅被单一线程(即单一活动对象 AO)访问,
 * 所以空闲槽数量不会意外变化.
 *
 * @param[in] me_ 指针
 *
 * @returns 队列当前可用的空闲槽数量。
 */
#define QEQueue_getNFree(me_) ((me_)->nFree)

/*!
 * "原始"线程安全 QF 事件队列操作: 获取队列中历史上最小的空闲槽数量
 * (即所谓的"低水位线").
 */
/**
 * @brief
 * 使用该操作时需要小心, 因为"低水位线"值也可能会意外变化.
 * 主要用途是帮助评估队列使用情况, 以便合理分配队列大小.
 *
 * @param[in] me_ 指针
 *
 * @returns 自初始化以来，队列中记录到的最小空闲槽数量。
 */
#define QEQueue_getNMin(me_) ((me_)->nMin)

/*!
 * "原始"线程安全 QF 事件队列操作: 判断队列是否为空.
 */
/**
 * @brief
 * 使用该操作时需要小心, 因为队列状态可能会意外变化.
 * 主要的使用场景是结合事件延迟(event deferral).
 * 在这种情况下, 队列仅被单一线程(即单一活动对象 AO)访问,
 * 所以队列状态不会意外变化.
 *
 * @param[in] me_ 指针
 *
 * @returns 如果队列当前为空返回 'true', 否则返回 'false'.
 */
#define QEQueue_isEmpty(me_) ((me_)->frontEvt == (QEvt *)0)

#endif /* QEQUEUE_H */
