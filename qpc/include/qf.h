/**
 * @file
 * @brief QF/C platform-independent public interface.
 * @ingroup qf
 * @cond
 ******************************************************************************
 * Last updated for version 6.9.3
 * Last updated on  2021-02-26
 *
 *                    Q u a n t u m  L e a P s
 *                    ------------------------
 *                    Modern Embedded Software
 *
 * Copyright (C) 2005-2021 Quantum Leaps, LLC. All rights reserved.
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
#ifndef QF_H
#define QF_H

#ifndef QPSET_H
#include "qpset.h"
#endif

/****************************************************************************/
#ifndef QF_EVENT_SIZ_SIZE
/*! 在 \b qf_port.h 中可配置宏的默认值 */
#define QF_EVENT_SIZ_SIZE 2U
#endif
#if (QF_EVENT_SIZ_SIZE == 1U)
typedef uint8_t QEvtSize;
#elif (QF_EVENT_SIZ_SIZE == 2U)
/*! 用于存储块大小的数据类型，依据宏 \b #QF_EVENT_SIZ_SIZE 定义 */
/**
 * 该数据类型的取值范围决定了内存池可管理的最大块大小。
 */
typedef uint16_t QEvtSize;
#elif (QF_EVENT_SIZ_SIZE == 4U)
typedef uint32_t QEvtSize;
#else
#error "QF_EVENT_SIZ_SIZE defined incorrectly, expected 1, 2, or 4"
#endif

#ifndef QF_MAX_EPOOL
/*! 在 \b qf_port.h 中可配置宏的默认值 */
#define QF_MAX_EPOOL 3U
#endif

#ifndef QF_MAX_TICK_RATE
/*! 在 \b qf_port.h 中可配置宏的默认值合法取值范围: [0U..15U]; 默认值为 1 */
#define QF_MAX_TICK_RATE 1U // hzh
#elif (QF_MAX_TICK_RATE > 15U)
#error "QF_MAX_TICK_RATE exceeds the maximum of 15"
#endif

#ifndef QF_TIMEEVT_CTR_SIZE
/*! 用于重定义 ::QTimeEvtCtr 大小的宏合法取值: 1, 2, 或 4; 默认值为 2 */
#define QF_TIMEEVT_CTR_SIZE 2U
#endif

/****************************************************************************/
struct QEQueue; /* forward declaration */

/****************************************************************************/

/*! QActive 活动对象基类 (基于 ::QHsm 实现)
 * @extends QHsm
 */
/**
 * @brief
 *  QP 中的活动对象是封装的状态机(每个活动对象都嵌入了一个事件队列和和一个线程), 它们通过发送
 * 和接收事件进行过异步通信. 在活动对象内部, 事件以RTC(run-to-completion )方式顺序处理.
 * 而QF 封装了线程安全事件交换和队列处理的所有细节.
 * @note
 *  ::QActive 表示一个采用 QHsm 风格实现策略的活动对象，该策略专为手动编码设计，但同样支持 QM 建模工具。
 * 生成的代码运行速度慢于 ::QMsm 风格实现策略。
 *  ::QActive 的成员 @c super 被定义为派生结构体的第一个成员
 */
typedef struct QActive {
    QHsm super; /*!< inherits ::QHsm */

#ifdef QF_EQUEUE_TYPE
    /*! 操作系统相关的事件队列类型 */
    /**
     * @brief
     *  队列的类型取决于底层操作系统或内核. 许多内核支持可适配的"消息队列",
     * 用于向活动对象传递QF事件. 此外，QF还提供原生事件队列实现方案，同样可供使用.
     * @note
     * 原生QF事件队列通过定义宏 \b #QF_EQUEUE_TYPE 为 ::QEQueue 来配置.
     */
    QF_EQUEUE_TYPE eQueue;
#endif

#ifdef QF_OS_OBJECT_TYPE
    /*! 操作系统相关的每线程对象 */
    /**
     * @brief
     *  该数据的使用方式可能因 QF port 而异. 在某些端口中, 当原生 QF 队列为空时,
     * OS-Object 用于阻塞调用线程. 而在其他 QF port 中，操作系统相关的对象可能
     * 具有不同的用途.
     */
    QF_OS_OBJECT_TYPE osObject;
#endif

#ifdef QF_THREAD_TYPE
    /*! 操作系统相关的活动对象线程表示 */
    /**
     * @brief
     * 该数据的使用方式可能因 QF port 而异. 在某些端口中，thread 用于存储线程句柄;
     * 而在其他端口中，thread 可能是指向线程局部存储区(TLS)的指针.
     */
    QF_THREAD_TYPE thread;
#endif

#ifdef QXK_H /* QXK kernel used? */
    /*! QXK dynamic priority (1..#QF_MAX_ACTIVE) of this AO/thread */
    uint8_t dynPrio;
#endif

    /*! QF priority (1..#QF_MAX_ACTIVE) of this active object. */
    uint8_t prio;

} QActive;

/*! ::QActive 类的虚表 */
typedef struct {
    struct QHsmVtable super; /*!< inherits ::QHsmVtable */

    /*! [virtual] 启动活动对象 (线程) */
    void (*start)(QActive *const me, uint_fast8_t prio,
                  QEvt const **const qSto, uint_fast16_t const qLen,
                  void *const stkSto, uint_fast16_t const stkSize,
                  void const *const par);

#ifdef Q_SPY
    /*! [virtual] 异步将事件 (以FIFO方式) post 到活动对象 */
    bool (*post)(QActive *const me, QEvt const *const e,
                 uint_fast16_t const margin, void const *const sender);
#else
    bool (*post)(QActive *const me, QEvt const *const e,
                 uint_fast16_t const margin);
#endif

    /*! [virtual] 异步将事件(以LIFO方式) post 到活动对象 */
    void (*postLIFO)(QActive *const me, QEvt const *const e);

} QActiveVtable;

/* QActive public operations... */
/*! 此以多态方式启动活动对象
 * @public @memberof QActive
 */
/**
 * @brief
 * 开始执行活动对象并将其注册到 QP 框架
 *
 * @param[in,out] me_      pointer (see @ref oop)
 * @param[in]     prio_    priority at which to start the active object
 * @param[in]     qSto_    pointer to the storage for the ring buffer of the
 *                         event queue (used only with the built-in ::QEQueue)
 * @param[in]     qLen_    length of the event queue (in events)
 * @param[in]     stkSto_  pointer to the stack storage (used only when
 *                         per-AO stack is needed)
 * @param[in]     stkSize_ stack size (in bytes)
 * @param[in]     par_     pointer to the additional port-specific parameter(s)
 *                         (might be NULL).
 */
#define QACTIVE_START(me_, prio_, qSto_, qLen_, stkSto_, stkLen_, par_) \
    do {                                                                \
        Q_ASSERT((Q_HSM_UPCAST(me_))->vptr);                            \
        (*((QActiveVtable const *)((Q_HSM_UPCAST(me_))->vptr))->start)( \
            (QActive *)(me_), (prio_),                                  \
            (qSto_), (qLen_), (stkSto_), (stkLen_), (par_));            \
    } while (false)

#ifdef Q_SPY
/*! 以多态方式向活动对象 (FIFO) 发布事件，并保证事件送达。
 * @public @memberof QActive
 */
/**
 * @brief 此宏用于断言队列是否已溢出且无法接受事件
 *
 * @param[in,out] me_   pointer (see @ref oop)
 * @param[in]     e_    pointer to the event to post
 * @param[in]     sender_ pointer to the sender object.
 *
 * @note 参数 @p sender_ 仅在 QS 跟踪启用时才使用 (macro \b #Q_SPY is defined).
 * 当 QS 软件跟踪禁用时, \b QACTIVE_POST() macro 不传递参数 @p sender_
 * 因此完全避免了传递此额外参数的开销
 *
 * @note @p sender_ 未必指向活动对象, 若在中断或其他上下文中调用 QACTIVE_POST(),
 * 则可以创建一个唯一对象明确标识事件的发送者.
 */
#define QACTIVE_POST(me_, e_, sender_)                                    \
    ((void)(*((QActiveVtable const *)((Q_HSM_UPCAST(me_))->vptr))->post)( \
        (me_), (e_), QF_NO_MARGIN, (sender_)))

/*! 多态方式向活动对象 (FIFO) 发布事件，不保证事件送达。
 * @public @memberof QActive
 */
/**
 * @brief 此宏在队列溢出, 无法接受事件且剩余空槽位不足时, 不会触发断言。
 *
 * @param[in,out] me_   pointer (see @ref oop)
 * @param[in]     e_    pointer to the event to post
 * @param[in]     margin_ 队列中在发布事件后仍需保留的最小空槽数.
 *                        特殊值 \b #QF_NO_MARGIN 表示若事件分配失败,
 *                        则触发断言错误.
 * @param[in]     sender_ pointer to the sender object.
 *
 * @returns 若发布成功返回 'true'，若因队列空槽不足导致发布失败返回 'false'。
 *
 * @note
 * 参数 @p sender_ 仅在启用 QS 追踪 (宏 \b #Q_SPY 被定义) 时使用.
 * 如果禁用了 QS 软件追踪，QACTIVE_POST() 宏不会传递 @p sender_,
 * 因此不会有额外的性能开销.
 *
 * @note
 * @p sender_ 指针不一定必须指向某个活动对象。实际上，
 * 如果 QACTIVE_POST_X() 在中断或其他上下文中被调用，
 * 你可以创建一个唯一的对象，只是为了明确标识事件的发送者。
 */
#define QACTIVE_POST_X(me_, e_, margin_, sender_)                         \
    ((*((QActiveVtable const *)((Q_HSM_UPCAST(me_))->vptr))->post)((me_), \
                                                                   (e_), (margin_), (sender_)))
#else

#define QACTIVE_POST(me_, e_, sender_)                                    \
    ((void)(*((QActiveVtable const *)((Q_HSM_UPCAST(me_))->vptr))->post)( \
        (me_), (e_), QF_NO_MARGIN))

#define QACTIVE_POST_X(me_, e_, margin_, sender_)                   \
    ((*((QActiveVtable const *)((Q_HSM_UPCAST(me_))->vptr))->post)( \
        (me_), (e_), (margin_)))

#endif

/*! 多态的向活动对象 (LIFO) 发布事件
 * @public @memberof QActive
 */
/**
 * @param[in,out] me_   pointer (see @ref oop)
 * @param[in]     e_    pointer to the event to post
 */
#define QACTIVE_POST_LIFO(me_, e_)                                      \
    ((*((QActiveVtable const *)((Q_HSM_UPCAST(me_))->vptr))->postLIFO)( \
        (me_), (e_)))

/* QActive protected operations... */
/*! protected "constructor" of an ::QActive active object
 * @protected @memberof QActive
 */
void QActive_ctor(QActive *const me, QStateHandler initial);

#ifdef QF_ACTIVE_STOP
/*! 停止活动对象的执行，并将其从框架的管理中移除
 * @protected @memberof QActive
 */
/** @attention
 * QActive_stop() 必须仅由即将停止执行的 AO 自身调用.
 * 调用该函数时, 任何指向该 AO 的指针或引用都被视为无效（悬空指针）,
 * 此时应用程序的其他部分向该 AO 投递事件已不再合法.
 */
void QActive_stop(QActive *const me);
#endif

/*! 订阅信号 @p sig, 以便传递给活动对象 @p me
 * @protected @memberof QActive
 */
void QActive_subscribe(QActive const *const me, enum_t const sig);

/*! 取消订阅信号 @p sig, 使其不再传递给活动对象 @p me
 * @protected @memberof QActive
 */
void QActive_unsubscribe(QActive const *const me, enum_t const sig);

/*! 取消订阅所有信号, 使其不再传递给活动对象 @p me
 * @protected @memberof QActive
 */
void QActive_unsubscribeAll(QActive const *const me);

/*! 将事件 @p e 延迟存储到指定的事件队列 @p eq 中
 * @protected @memberof QActive
 */
bool QActive_defer(QActive const *const me, QEQueue *const eq, QEvt const *const e);

/*! 从指定的事件队列 @p eq 中取回一个之前延迟的事件
 * @protected @memberof QActive
 */
bool QActive_recall(QActive *const me, QEQueue *const eq);

/*! 清空指定的延迟队列 @p eq
 * @protected @memberof QActive
 */
uint_fast16_t QActive_flushDeferred(QActive const *const me, QEQueue *const eq);

/*! 通用的附加属性设置 (useful in QP ports)
 * @protected @memberof QActive
 */
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2);

/****************************************************************************/
/*! QMActive 活动对象基类(基于 ::QMsm 实现)
 * @extends QActvie
 */
/**
 * @brief
 * QMActive 表示一种使用 ::QMsm 风格状态机实现策略的活动对象.
 * 这种策略需要使用 QM 建模工具自动生成状态机代码,
 * 但与 ::QHsm 风格的实现策略相比，生成的代码执行更快,
 * 并且需要的运行时支持更少 (事件处理器更小).
 *
 * @note
 * ::QMActive 并不打算直接实例化, 而是作为应用程序中活动对象的基类使用
 */
typedef struct {
    QActive super; /*!< inherits ::QActive */
} QMActive;

/*! ::QMActive 类的虚表(继承自 ::QActiveVtable) */
/**
 * @note
 * ::QMActive 完全继承了 ::QActive，没有新增任何虚函数，
 * 因此，::QMActiveVtable 被 typedef 为 ::QActiveVtable。
 */
typedef QActiveVtable QMActiveVtable;

/* QMActive protected operations... */
/*! protected "constructor" of an ::QMActive active object.
 * @protected @memberof QMActive
 */
void QMActive_ctor(QMActive *const me, QStateHandler initial);

/****************************************************************************/
#if (QF_TIMEEVT_CTR_SIZE == 1U)
typedef uint8_t QTimeEvtCtr;
#elif (QF_TIMEEVT_CTR_SIZE == 2U)

/*! 时间事件计数器的类型, 它决定了以时钟节拍为单位测量的时间延迟范围  */
/**
 * @brief
 * 该 typedef 可通过预处理器开关 \b #QF_TIMEEVT_CTR_SIZE 配置。
 * 该类型的其他可能取值如下: @n
 * 当 (QF_TIMEEVT_CTR_SIZE == 1U) 时为 uint8_t,
 * 当 (QF_TIMEEVT_CTR_SIZE == 2U) 时为 uint16_t,
 * 当 (QF_TIMEEVT_CTR_SIZE == 4U) 时为 uint32_t.
 */
typedef uint16_t QTimeEvtCtr;
#elif (QF_TIMEEVT_CTR_SIZE == 4U)
typedef uint32_t QTimeEvtCtr;
#else
#error "QF_TIMEEVT_CTR_SIZE defined incorrectly, expected 1, 2, or 4"
#endif

/*! Time Event class
 * @extends QEvt
 */
/**
 * @brief
 * 时间事件是一种特殊的 QF 事件，带有时间流逝的概念.
 * 时间事件的基本使用模型如下:
 * 一个活动对象分配一个或多个 ::QTimeEvt 对象(提供它们的存储空间).
 * 当活动对象需要安排超时时, 它会启动其中一个时间事件,
 * 该事件可以是单次触发(one-shot), 也可以是周期性触发.
 * 每个时间事件独立超时, 因此 QF 应用可以同时提出多个并行的超时请求(来自相同或不同的活动对象)
 * 当 QF 检测到合适的时机到来时, 它会将时间事件直接插入到接收者的事件队列中.
 * 接收者随后会像处理其他事件一样处理该时间事件.
 *
 * 时间事件和其他 QF 事件一样，派生自 ::QEvt 基类.
 * 通常情况下你会直接使用时间事件, 但你也可以通过添加更多的数据成员
 * 和/或专门的函数来进一步派生出更特化的时间事件。
 *
 * 在内部，已启动的时间事件会按定时速率被组织成链表——每个支持的 tick 速率对应一个链表。
 * 在每次调用 QF_tickX_() 函数时，都会扫描这些链表。
 * 只有已启动（正在计时）的时间事件会在链表中，因此只有它们会消耗 CPU 周期。
 *
 * @note
 * QF 在函数 QF_tickX_() 中管理时间事件, 该函数必须被周期性调用.
 * 调用来源可以是时钟节拍 ISR 或专用的 ::QTicker 活动对象.
 *
 * @note
 * 尽管 ::QTimeEvt 是 ::QEvt 的子类，但 ::QTimeEvt 实例不能从事件池动态分配.
 * 换句话说, 使用 Q_NEW() 或 Q_NEW_X() 宏来分配 ::QTimeEvt 实例是非法的.
 */
typedef struct QTimeEvt {
    QEvt super; /*<! inherits ::QEvt */

    /*! 指向链表中下一个时间事件 */
    struct QTimeEvt *volatile next;

    /*! 接收时间事件的活动对象 */
    void *volatile act;

    /**
     * @brief 时间事件的内部递减计数器
     * 在每次调用 QF_tickX_() 时, 该计数器会减 1.
     * 当计数器减到零时, 时间事件触发(被投递或发布)
     */
    QTimeEvtCtr volatile ctr;

    /**
     * @brief 周期性时间事件的间隔(单次事件时为零)
     * 当时间事件到期时, 该值会重新加载到内部计数器中, 这样时间事件就会周期性超时.
     */
    QTimeEvtCtr interval;
} QTimeEvt;

/* QTimeEvt public operations... */

/*! 构造函数, 初始化时间事件
 * @public @memberof QTimeEvt
 */
void QTimeEvt_ctorX(QTimeEvt *const me, QActive *const act,
                    enum_t const sig, uint_fast8_t tickRate);

/*! 启动一个时间事件(单次或周期性),并直接投递事件
 * @public @memberof QTimeEvt
 */
void QTimeEvt_armX(QTimeEvt *const me,
                   QTimeEvtCtr const nTicks, QTimeEvtCtr const interval);

/*! 重新启动一个时间事件
 * @public @memberof QTimeEvt
 */
bool QTimeEvt_rearm(QTimeEvt *const me, QTimeEvtCtr const nTicks);

/*! 取消启动一个时间事件
 * @public @memberof QTimeEvt
 */
bool QTimeEvt_disarm(QTimeEvt *const me);

/*! 检查时间事件是否"被取消"
 * @public @memberof QTimeEvt
 */
bool QTimeEvt_wasDisarmed(QTimeEvt *const me);

/*! 获取时间事件当前的递减计数器值
 * @public @memberof QTimeEvt
 */
QTimeEvtCtr QTimeEvt_currCtr(QTimeEvt const *const me);

/****************************************************************************/
/* QF facilities */

/*! Subscriber-List Structure */
/**
 * @brief
 * 该数据类型表示一组订阅了某个信号的活动对象.
 * 该集合以优先级集合 (priority-set) 的形式表示,
 * 其中每一位对应一个活动对象的唯一优先级.
 */
typedef QPSet QSubscrList;

/* public functions */

/*! QF 初始化 */
void QF_init(void);

/*! 发布-订阅机制初始化 */
void QF_psInit(QSubscrList *const subscrSto, enum_t const maxSignal);

/*! 事件池初始化，用于事件的动态分配 */
void QF_poolInit(void *const poolSto, uint_fast32_t const poolSize, uint_fast16_t const evtSize);

/*! 获取任意已注册事件池的块大小 */
uint_fast16_t QF_poolGetMaxBlockSize(void);

/*! 将控制权交给 QF 以运行应用程序 */
int_t QF_run(void);

/*! 应用层调用该函数以停止 QF 应用程序，并将控制权交还给 OS/内核 */
void QF_stop(void);

/*! Startup QF callback. */
/**
 * @brief
 * 调用 QF_onStartup() 的时机取决于具体的 QF 移植,
 * 在大多数情况下, QF_onStartup() 会在 QF_run() 中被调用,
 * 即在启动任何多任务内核或后台循环之前.
 */
void QF_onStartup(void);

/*! Cleanup QF callback. */
/**
 * @brief 在某些 QF 移植中, QF_onCleanup() 会在 QF 返回到底层操作系统或 RTOS 之前调用.
 *
 * 此函数具有强烈的平台相关性, 它不在 QF 中实现, 而是在 QF 移植层或 BSP(板级支持包)中实现.
 * 部分 QF 移植可能完全不需要实现 QF_onCleanup(), 因为很多嵌入式应用并没有“退出”的概念.
 */
void QF_onCleanup(void);

#ifdef Q_SPY

/*! Publish event to the framework. */
void QF_publish_(QEvt const *const e,
                 void const *const sender, uint_fast8_t const qs_id);

/*! Invoke the event publishing facility QF_publish_(). */
/**
 * @brief
 * 该宏是发布事件的推荐方式, 因为它为软件追踪提供了重要信息,
 * 并且在追踪被禁用时避免了额外开销
 *
 * @param[in] e_      pointer to the posted event
 * @param[in] sender_ 指向发送者对象的指针. 该参数仅在启用
 *            QS 软件追踪时使用(定义了宏 \b #Q_SPY),当禁用
 *            QS 软件追踪时, 该宏会调用 QF_publish_()
 *            且不传递 @p sender_ 参数, 因此不会有额外开销.
 * @note
 * @p sender_ 指针不一定必须指向某个活动对象.
 * 实际上, 如果 QF_PUBLISH() 在中断或其他上下文中被调用,
 * 你可以创建一个唯一对象, 用来明确标识事件发布者.
 */
#define QF_PUBLISH(e_, sender_) \
    (QF_publish_((e_), (void const *)(sender_), (sender_)->prio))

#else

void QF_publish_(QEvt const *const e);
#define QF_PUBLISH(e_, dummy_) (QF_publish_(e_))

#endif

#ifdef Q_SPY

/*! 在每个时钟节拍中处理所有已启动的时间事件 */
void QF_tickX_(uint_fast8_t const tickRate, void const *const sender);

/*! 调用系统时钟节拍处理函数 QF_tickX_() */
/**
 * @brief
 * 该宏是调用时钟节拍处理的推荐方式, 因为它为软件追踪提供了关键信息,
 * 并且在追踪被禁用时避免了额外开销.
 *
 * @param[in] tickRate_ 需要服务的时钟节拍速率
 * @param[in] sender_    指向发送者对象的指针.
 *            该参数仅在启用 QS 软件追踪时使用 (定义了宏 \b #Q_SPY)

 * @note
 * 当禁用 QS 软件追踪时, 该宏会调用 QF_tickX_(), 且不会传递 @p sender 参数,
 * 因此不会有额外的性能开销.
 *
 * @note
  * @p sender_ 指针不一定必须指向某个活动对象.
 * 实际上, 当 \b #QF_TICK_X() 在中断中被调用时, 可以创建一个唯一对象,
 * 用来明确标识该 ISR 是时间事件的发送者.
 */
#define QF_TICK_X(tickRate_, sender_) (QF_tickX_((tickRate_), (sender_)))

#else

void QF_tickX_(uint_fast8_t const tickRate);
#define QF_TICK_X(tickRate_, dummy) (QF_tickX_(tickRate_))

#endif

/*! 特殊的 margin 值, 当事件分配或事件发布失败时会触发断言错误 */
#define QF_NO_MARGIN ((uint_fast16_t)0xFFFFU)

/*! 调用系统时钟节拍处理 (速率为 0) */
#define QF_TICK(sender_) QF_TICK_X(0U, (sender_))

/*! 如果在指定的时钟速率下没有已启动的时间事件, 则返回 'true' */
bool QF_noTimeEvtsActiveX(uint_fast8_t const tickRate);

/*! 注册一个活动对象，使其由框架管理 */
void QF_add_(QActive *const a);

/*! 将活动对象从框架中移除 */
void QF_remove_(QActive *const a);

/*! 获取指定事件池的最小剩余空闲条目数 */
uint_fast16_t QF_getPoolMin(uint_fast8_t const poolId);

/*! 获取指定事件队列的最小剩余空闲条目数 */
uint_fast16_t QF_getQueueMin(uint_fast8_t const prio);

/*! 内部 QF 实现: 创建新的动态事件 */
QEvt *QF_newX_(uint_fast16_t const evtSize, uint_fast16_t const margin, enum_t const sig);

/*! 内部 QF 实现: 创建新的事件引用 */
QEvt const *QF_newRef_(QEvt const *const e, void const *const evtRef);

/*! 内部 QF 实现: 删除事件引用 */
void QF_deleteRef_(void const *const evtRef);

#ifdef Q_EVT_CTOR /* 可变参数宏 是否为 ::QEvt 类提供构造函数? */

#define Q_NEW(evtT_, sig_, ...)                                   \
    (evtT_##_ctor((evtT_ *)QF_newX_((uint_fast16_t)sizeof(evtT_), \
                                    QF_NO_MARGIN, 0),             \
                  (sig_), ##__VA_ARGS__))

#define Q_NEW_X(e_, evtT_, margin_, sig_, ...)                 \
    do {                                                       \
        (e_) = (evtT_ *)QF_newX_((uint_fast16_t)sizeof(evtT_), \
                                 (margin_), 0);                \
        if ((e_) != (evtT_ *)0) {                              \
            evtT_##_ctor((e_), (sig_), ##__VA_ARGS__);         \
        }                                                      \
    } while (false)

#else

/*! 分配一个动态事件 (断言版本) */
/**
 * @brief
 * 该宏调用 QF 内部函数 QF_newX_(), 并传入参数 margin == \b #QF_NO_MARGIN.
 * 如果事件分配失败, 会触发断言错误.
 *
 * @param[in] evtT_ 要分配的事件类型 (类名)
 * @param[in] sig_  要分配的事件的信号
 *
 * @returns 转换为 @p evtT_ 类型的有效事件指针
 *
 * @note
 * 如果定义了 Q_EVT_CTOR, Q_NEW() 宏会变成可变参数宏，
 * 接收事件类构造函数所需的全部参数. 构造函数会通过 placement-new 方式被调用。
 */
#define Q_NEW(evtT_, sig_)                           \
    ((evtT_ *)QF_newX_((uint_fast16_t)sizeof(evtT_), \
                       QF_NO_MARGIN, (sig_)))

/*! 分配一个动态事件 (非断言版本) */
/**
 * @brief
 * 该宏分配一个新事件并设置指针 @p e_,
 * 同时保证事件池中至少剩余 @p margin_ 个事件未被分配.
 * 如果使用特殊值 \b #QF_NO_MARGIN, 则在事件分配或发布失败时触发断言.
 *
 * @param[in,out] e_  指向新分配事件的指针
 * @param[in] evtT_   要分配的事件类型 (类名)
 * @param[in] margin_ 分配完成后事件池中必须至少剩余的事件数量.
 *                    特殊值 \b #QF_NO_MARGIN 表示如果分配或发布失败,
 *                    则会触发断言错误.
 * @param[in] sig_    要分配的事件的信号
 *
 * @returns 转换为 @p evtT_ 类型的事件指针:
 *          如果在给定的 @p margin_ 条件下无法分配事件, 则返回 NULL.
 *
 * @note
 * 如果定义了 Q_EVT_CTOR，Q_NEW_X() 宏会变成可变参数宏,
 * 接收事件类构造函数所需的全部参数.
 * 构造函数会被调用, 并传入这些额外的参数.
 */
#define Q_NEW_X(e_, evtT_, margin_, sig_) ((e_) = \
                                               (evtT_ *)QF_newX_((uint_fast16_t)sizeof(evtT_), (margin_), (sig_)))

#endif /* Q_EVT_CTOR */

/*! 创建当前事件 `e` 的新引用 */
/**
 * @brief
 * 活动对象正在处理的当前事件只在 run-to-completion (RTC) 步骤中有效.
 * 该步骤结束后, 当前事件将不再可用, 框架可能会回收（垃圾收集）该事件.
 * 宏 Q_NEW_REF() 明确创建了一个对当前事件的新引用, 可以存储并在当前
 * RTC 步骤结束后继续使用, 直到通过宏 Q_DELETE_REF() 显式回收该引用.
 *
 * @param[in,out] evtRef_  要创建的事件引用
 * @param[in]     evtT_    事件引用的事件类型 (类名)
 */
#define Q_NEW_REF(evtRef_, evtT_) \
    ((evtRef_) = (evtT_ const *)QF_newRef_(e, (evtRef_)))

/*! 删除事件引用 */
/**
 * @brief
 * 使用宏 Q_NEW_REF() 创建的每个事件引用最终都需要通过 Q_DELETE_REF() 删除,
 * 以避免事件泄漏.
 *
 * @param[in,out] evtRef_  要删除的事件引用
 */
#define Q_DELETE_REF(evtRef_)     \
    do {                          \
        QF_deleteRef_((evtRef_)); \
        (evtRef_) = (void *)0;    \
    } while (false)

/*! 回收一个动态事件 */
void QF_gc(QEvt const *const e);

/*! 将指定内存区域清零 */
void QF_bzero(void *const start, uint_fast16_t len);

#ifndef QF_CRIT_EXIT_NOP
/**
 * @brief 退出临界区的空操作
 * 在某些 QF 移植版本中, 退出临界区的操作只有在下一条机器指令执行时才生效.
 * 如果下一条指令又是进入临界区, 那么临界区实际上不会被真正退出,两个相邻的
 * 临界区会被合并. 宏 \b #QF_CRIT_EXIT_NOP() 包含最小的代码, 用于防止在可
 * 能出现此情况的 QF 移植版本中临界区被意外合并.
 */
#define QF_CRIT_EXIT_NOP() ((void)0)
#endif

/*! 已注册活动对象数组 */
/**
 * @note 客户端不要直接使用, 仅用于 QF 移植层
 */
extern QActive *QF_active_[QF_MAX_ACTIVE + 1U];

/****************************************************************************/
/*! QTicker 活动对象类
 * @extends QActive
 */
/**
 * @brief
 * QTicker 是一个高效的活动对象, 专门用于以指定 tick 频率 [0..::QF_MAX_TICK_RATE]
 * 处理 QF 系统时钟节拍.
 * 将系统时钟节拍处理放在活动对象中, 可以将非确定性的 QF::TICK_X()
 * 中断级处理移到线程级别, 从而可以将其优先级设置得尽可能低.
 */
typedef struct {
    QActive super; /*!< inherits ::QActive */
} QTicker;

/*! QTicker 活动对象类的构造函数 */
void QTicker_ctor(QTicker *const me, uint_fast8_t tickRate);

#endif /* QF_H */
