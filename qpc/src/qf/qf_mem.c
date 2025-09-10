/**
 * @file
 * @ingroup qf
 * @brief ::QMPool implementatin (Memory Pool)
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.1
 * Last updated on  2020-09-03
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

Q_DEFINE_THIS_MODULE("qf_mem")

/****************************************************************************/
/**
 * @brief
 * 初始化一个固定块大小的内存池, 通过提供池要管理的内存, 内存大小和块大小.
 *
 * @param[in,out] me       指向 QMPool 对象的指针
 * @param[in]     poolSto  指向内存缓冲区, 用于池的存储
 * @param[in]     poolSize 存储缓冲区的大小(字节)
 * @param[in]     blockSize 内存块的固定大小(字节)
 *
 * @attention
 * 调用 QMPool::init() 的用户必须确保 @p poolSto 指针正确 \b 对齐
 * 特别是, 该地址必须能高效存储一个指针.
 * QMPool_init() 内部会将块大小 @p blockSize 向上取整, 使其能容纳整数个指针,
 * 以保证池内块的正确对齐.
 *
 * @note
 * 由于块大小取整, 池的实际容量可能小于 ( @p poolSize / @p blockSize)
 * 可以通过调用 QF_getPoolMin() 检查池的容量
 *
 * @note
 * 此函数 \b 不受临界区保护, 因为它只在系统初始化时调用, 此时中断尚未开启
 *
 * @note
 * 许多 QF 移植层使用内存池来实现事件池
 */
void QMPool_init(QMPool *const me, void *const poolSto,
                 uint_fast32_t poolSize, uint_fast16_t blockSize)
{
    QFreeBlock *fb;
    uint_fast16_t nblocks;

    /** @pre 内存块必须有效
     * 且 poolSize 必须至少能容纳一个空闲块
     * 且 blockSize 不应接近动态范围的上限
     */
    Q_REQUIRE_ID(100, (poolSto != (void *)0) && (poolSize >= sizeof(QFreeBlock)) && ((blockSize + sizeof(QFreeBlock)) > blockSize));

    me->free_head = poolSto;

    /* 将 blockSize 向上取整以适应整数个空闲块, 无需除法 */
    me->blockSize = (QMPoolSize)sizeof(QFreeBlock); /* 从一个块开始 */
    nblocks       = 1U;                             /* 一个内存块能容纳的空闲块数 */
    while (me->blockSize < (QMPoolSize)blockSize) {
        me->blockSize += (QMPoolSize)sizeof(QFreeBlock);
        ++nblocks;
    }
    blockSize = (uint_fast16_t)me->blockSize; /* 向上取整到最接近的块大小 */

    /* 池缓冲区必须至少能容纳一个取整后的块 */
    Q_ASSERT_ID(110, poolSize >= blockSize);

    /* 将所有块链成空闲链表... */
    poolSize -= (uint_fast32_t)blockSize;   /* 不算最后一个块 */
    me->nTot = 1U;                          /* 最后一个块已在池中 */
    fb       = (QFreeBlock *)me->free_head; /* 从空闲链表头开始 */

    /* 将所有块链成空闲链表 */
    while (poolSize >= (uint_fast32_t)blockSize) {
        fb->next = &QF_PTR_AT_(fb, nblocks);  /* 将 next 指向下一个块 */
        fb       = fb->next;                  /* 移动到下一个块 */
        poolSize -= (uint_fast32_t)blockSize; /* 减少可用池大小 */
        ++me->nTot;                           /* 累计块数 */
    }

    fb->next  = (QFreeBlock *)0; /* 最后一个块的 next 指向 NULL */
    me->nFree = me->nTot;        /* 所有块均为可用 */
    me->nMin  = me->nTot;        /* 最小空闲块数 */
    me->start = poolSto;         /* 池缓冲区起始地址 */
    me->end   = fb;              /* 池内最后一个块 */
}

/****************************************************************************/
/**
 * @brief
 * 将一个内存块放回到固定块大小的内存池中
 *
 * @param[in,out] me   指向 QMPool 对象的指针
 * @param[in]     b    指向要回收的内存块的指针
 *
 * @attention
 * 回收的内存块必须是从 \b 同一个 内存池分配
 *
 * @note
 * 此函数可以从任何任务级别或中断服务程序(ISR)调用
 */
void QMPool_put(QMPool *const me, void *b, uint_fast8_t const qs_id)
{
    QF_CRIT_STAT_

    /** @pre 空闲块数不能超过总块数, 并且块指针必须属于该内存池 */
    Q_REQUIRE_ID(200, (me->nFree < me->nTot) && QF_PTR_RANGE_(b, me->start, me->end));

    (void)qs_id; /* unused parameter (outside Q_SPY build configuration) */

    QF_CRIT_E_();
    ((QFreeBlock *)b)->next = (QFreeBlock *)me->free_head; /* 链接到空闲链表 */
    me->free_head           = b;                           /* 设置为空闲链表的新头 */
    ++me->nFree;                                           /* 空闲块数加 1 */

    QS_BEGIN_NOCRIT_PRE_(QS_QF_MPOOL_PUT, qs_id)
    QS_TIME_PRE_();         /* 时间戳 */
    QS_OBJ_PRE_(me);        /* 内存池对象 */
    QS_MPC_PRE_(me->nFree); /* 内存池中空闲块的数量 */
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();
}

/****************************************************************************/
/**
 * @brief
 * 从内存池中分配一个内存块, 并返回该块的指针给调用者
 *
 * @param[in,out] me      指向 QMPool 对象的指针
 * @param[in]     margin  分配完成后, 池中仍需保留的最小空闲块数量
 *
 * @note
 * 此函数可以从任何任务级别或中断服务程序(ISR)调用
 *
 * @note
 * 内存池 @p me 必须在请求任何内存块之前初始化.
 * QMPool_get() 内部使用了 QF 临界区, 因此如果平台不支持临界区嵌套,
 * 请注意不要在已有临界区内再次调用它.
 *
 * @attention
 * 分配的内存块必须在使用完毕后返回到同一个内存池
 */
void *QMPool_get(QMPool *const me, uint_fast16_t const margin,
                 uint_fast8_t const qs_id)
{
    QFreeBlock *fb;
    QF_CRIT_STAT_

        (void)
    qs_id; /* unused parameter (outside Q_SPY build configuration) */

    QF_CRIT_E_();

    /* 是否有足够的空闲块超过请求的 margin? */
    if (me->nFree > (QMPoolCtr)margin) {
        void *fb_next;
        fb = (QFreeBlock *)me->free_head; /* 获取一个空闲块 */

        /* 内存池至少有一个空闲块 */
        Q_ASSERT_CRIT_(310, fb != (QFreeBlock *)0);

        fb_next = fb->next; /* 临时存储以避免未定义行为 */

        /* 内存池是否即将耗尽? */
        --me->nFree; /* 空闲块数量减 1 */
        if (me->nFree == 0U) {
            /* 池已空, 下一空闲块必须为 NULL */
            Q_ASSERT_CRIT_(320, fb_next == (QFreeBlock *)0);

            me->nMin = 0U; /* 记录池曾空 */
        } else {
            /* 池未空, 下一空闲块必须在有效范围内
             * 注意: 若用户代码越界写入, 则可能破坏下一块指针.
             */
            Q_ASSERT_CRIT_(330, QF_PTR_RANGE_(fb_next, me->start, me->end));

            /* 更新空闲块数量的历史最小值 */
            if (me->nMin > me->nFree) {
                me->nMin = me->nFree; /* 记录新最小值 */
            }
        }

        me->free_head = fb_next; /* 更新空闲链表头 */

        QS_BEGIN_NOCRIT_PRE_(QS_QF_MPOOL_GET, qs_id)
        QS_TIME_PRE_();         /* 时间戳 */
        QS_OBJ_PRE_(me);        /* 内存池对象 */
        QS_MPC_PRE_(me->nFree); /* 当前空闲块数量 */
        QS_MPC_PRE_(me->nMin);  /* 历史最小空闲块数量 */
        QS_END_NOCRIT_PRE_()
    } else {
        /* 空闲块不足 */
        fb = (QFreeBlock *)0;

        QS_BEGIN_NOCRIT_PRE_(QS_QF_MPOOL_GET_ATTEMPT, qs_id)
        QS_TIME_PRE_();         /* 时间戳 */
        QS_OBJ_PRE_(me);        /* 内存池对象 */
        QS_MPC_PRE_(me->nFree); /* 当前空闲块数量 */
        QS_MPC_PRE_(margin);    /* 请求的 margin */
        QS_END_NOCRIT_PRE_()
    }
    QF_CRIT_X_();

    return fb; /* 返回分配块或 NULL 指针 */
}

/****************************************************************************/
/**
 * @brief
 * 此函数用于获取自从调用 QF_poolInit() 初始化以来, 指定事件池中剩余空闲块数量的最小值.
 *
 * @param[in] poolId  事件池 ID, 取值范围为 1..QF_maxPool_,
 *                    其中 QF_maxPool_ 表示通过 QF_poolInit()初始化的事件池数量
 *
 * @returns
 * 指定事件池中曾经最少的未使用块数量
 */
uint_fast16_t QF_getPoolMin(uint_fast8_t const poolId)
{
    uint_fast16_t min;
    QF_CRIT_STAT_

    /** @pre poolId 必须在有效范围内 */
    Q_REQUIRE_ID(400, (0U < poolId) && (poolId <= QF_maxPool_) && (poolId <= QF_MAX_EPOOL));

    QF_CRIT_E_();
    min = (uint_fast16_t)QF_pool_[poolId - 1U].nMin;
    QF_CRIT_X_();

    return min;
}
