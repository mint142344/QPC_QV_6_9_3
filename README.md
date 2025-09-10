# [QPC](https://github.com/QuantumLeaps/qpc/tree/v6.9.3)

- QF ports：QF 移植
- native ：裸机

- 使用QF_NO_MARGIN的函数会有断言失败机制



集成qpc（qv）所需文件

**include**：

**ports/arm-cm/qv/arm/**：

**src**：

>  NOTE
>
> 1. 用到事件 包含**"qep_port.h"** 而不是 **qep.h**
>
> 2. 用到活动对象 包含**"qpc.h"**
> 3. **不需要修改qpc源代码文件**

## include

### qassert.h

```c

```

### qep.h

```c
typedef struct {
    QSignal sig;              /*!< 事件实例的信号 */
    uint8_t poolId_;          /*!< 所属事件池 ID（静态事件为 0）*/
    uint8_t volatile refCtr_; /*!< 引用计数器 */
} QEvt;
```

- `poolId_` 为0表示静态事件，此时`refCtr_`不用于引用计数

### qequeue.h

```c

```

### qf.h

```c
#include "qpset.h"
```

- QActive

```c
// 活动对象基类 (基于 ::QHsm 实现)
struct QActive {
    QHsm super;
    QEQueue eQueue;
    uint8_t prio;
};

// QActive 类虚表
struct QActiveVtable {
	// [virtual] 启动活动对象
    void (*start)(QActive *const me, uint_fast8_t prio,
                  QEvt const **const qSto, uint_fast16_t const qLen,
                  void *const stkSto, uint_fast16_t const stkSize,
                  void const *const par);
    // [virtual] FIFO 异步发送事件给活动对象
    bool (*post)(QActive *const me, QEvt const *const e,
                 uint_fast16_t const margin);
    // [virtual] LIFO 异步发送事件给活动对象
    void (*postLIFO)(QActive *const me, QEvt const *const e);
};


// QM 建模工具使用
typedef struct {
    QActive super;
} QMActive;
typedef QActiveVtable QMActiveVtable;
void QMActive_ctor(QMActive *const me, QStateHandler initial);


// QTicker 是一个高效的活动对象, 专门用于以指定 tick 频率
typedef struct {
    QActive super; /*!< inherits ::QActive */
} QTicker;
void QTicker_ctor(QTicker *const me, uint_fast8_t tickRate);
```

public 函数

```c
// QActive public 函数
#define QACTIVE_START(...)
#define QACTIVE_POST(...)   	// 不会断言失败(FIFO)
#define QACTIVE_POST_X(...) 	// 会断言失败(FIFO)
#define QACTIVE_POST_LIFO(...)
```

protected 函数

```c
// QActive protected 函数
void QActive_ctor(QActive *const me, QStateHandler initial);
void QActive_stop(QActive *const me);

// 订阅信号 @p sig, 以便传递给活动对象 @p me
void QActive_subscribe(QActive const *const me, enum_t const sig);

// 取消订阅信号 @p sig, 使其不再传递给活动对象 @p me
void QActive_unsubscribe(QActive const *const me, enum_t const sig);

// 取消订阅所有信号, 使其不再传递给活动对象 @p me
void QActive_unsubscribeAll(QActive const *const me);

// 将事件 @p e 延迟存储到指定的事件队列 @p eq 中
bool QActive_defer(QActive const *const me, QEQueue *const eq, QEvt const *const e);

// 从指定的事件队列 @p eq 中取回一个之前延迟的事件
bool QActive_recall(QActive *const me, QEQueue *const eq);

// 清空指定的延迟队列 @p eq
uint_fast16_t QActive_flushDeferred(QActive const *const me, QEQueue *const eq);

// 通用的附加属性设置 (useful in QP ports)
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2);
```

- QTimeEvt 事件事件类

```c
typedef struct QTimeEvt {
    QEvt super; 						// inherits ::QEvt
    struct QTimeEvt *volatile next;		// 指向链表中下一个时间事件
    void *volatile act;     			// 接收时间事件的活动对象
    QTimeEvtCtr volatile ctr;    		// 计数器
    QTimeEvtCtr interval;    			// 重载值
} QTimeEvt;
```

public 函数

```c
// 构造函数, 初始化时间事件
void QTimeEvt_ctorX(QTimeEvt *const me, QActive *const act,
                    enum_t const sig, uint_fast8_t tickRate);

// 启动一个时间事件(单次或周期性),并直接投递事件
void QTimeEvt_armX(QTimeEvt *const me,
                   QTimeEvtCtr const nTicks, QTimeEvtCtr const interval);

// 重新启动一个时间事件
bool QTimeEvt_rearm(QTimeEvt *const me, QTimeEvtCtr const nTicks);

// 取消启动一个时间事件
bool QTimeEvt_disarm(QTimeEvt *const me);

// 检查时间事件是否"被取消"
bool QTimeEvt_wasDisarmed(QTimeEvt *const me);

// 获取时间事件当前的递减计数器值
QTimeEvtCtr QTimeEvt_currCtr(QTimeEvt const *const me);
```

- QF facilities

```c
// 订阅列表
typedef QPSet QSubscrList;

/* public functions */
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


// QF 回调
void QF_onStartup(void);
void QF_onCleanup(void);


// 事件发布
void QF_publish_(QEvt const *const e);
#define QF_PUBLISH(e_, dummy_) (QF_publish_(e_))


// 在时钟节拍中处理事件事件
void QF_tickX_(uint_fast8_t const tickRate);
#define QF_TICK_X(tickRate_, dummy) (QF_tickX_(tickRate_))
// 如果在指定的时钟速率下没有已启动的时间事件, 则返回 'true'
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
// 分配一个动态事件 (断言版本)
#define Q_NEW(evtT_, sig_)
// 分配一个动态事件 (非断言版本)
#define Q_NEW_X(e_, evtT_, margin_, sig_)

/*! 内部 QF 实现: 创建新的事件引用 */
QEvt const *QF_newRef_(QEvt const *const e, void const *const evtRef);
/*! 内部 QF 实现: 删除事件引用 */
void QF_deleteRef_(void const *const evtRef);
// 创建当前事件 `e` 的新引用
#define Q_NEW_REF(evtRef_, evtT_)
// 删除事件引用
#define Q_DELETE_REF(evtRef_)
```

### qmpool.h

```c

```

### qpc.h

```c
#include "qf_port.h"      /* QF/C port from the port directory */
#include "qassert.h"      /* QP embedded systems-friendly assertions */
```

### qpset.h

```c

```

### qv.h

协作式内核

```c
#include "qequeue.h"  /* QV kernel uses the native QP event queue  */
#include "qmpool.h"   /* QV kernel uses the native QP memory pool  */
#include "qpset.h"    /* QV kernel uses the native QP priority set */

#define QF_EQUEUE_TYPE      QEQueue
// QF_run() 中调用
void QV_onIdle(void);

/* QV 内核特有的调度器加锁机制 (但在 QV 中不需要) */
#define QF_SCHED_STAT_
#define QF_SCHED_LOCK_(dummy)   ((void)0)
#define QF_SCHED_UNLOCK_(dummy) ((void)0)

/* QF 原生事件队列操作 */
#define QACTIVE_EQUEUE_WAIT_(me_) \
    Q_ASSERT_ID(0, (me_)->eQueue.frontEvt != (QEvt *)0)

#define QACTIVE_EQUEUE_SIGNAL_(me_) \
    QPSet_insert(&QV_readySet_, (uint_fast8_t)(me_)->prio)



/* QF 原生事件池操作 */
#define QF_EPOOL_TYPE_ QMPool

// QF_EPOOL_INIT_ 事件池初始化
#define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
    (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))

// QF_EPOOL_EVENT_SIZE_ 事件池大小
#define QF_EPOOL_EVENT_SIZE_(p_) ((uint_fast16_t)(p_).blockSize)

// QF_EPOOL_GET_	
#define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
    ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qs_id_)))
    
// QF_EPOOL_PUT_
#define QF_EPOOL_PUT_(p_, e_, qs_id_) \
    (QMPool_put(&(p_), (e_), (qs_id_)))
```

### qpc.h

```c
#include "qf_port.h"      /* QF/C port from the port directory */
#include "qassert.h"      /* QP embedded systems-friendly assertions */
```

## ports

移植目录

### qep_port.h

```c
#include <stdint.h>  /* Exact-width types. WG14/N843 C99 Standard */
#include <stdbool.h> /* Boolean type.      WG14/N843 C99 Standard */
#include "qep.h"     /* QEP platform-independent public interface */
```

### qf_port.h

```c
#include "qep_port.h" /* QEP port */
#include "qv_port.h"  /* QV cooperative kernel port */
#include "qf.h"       /* QF platform-independent public interface */
```

宏  **QF_AWARE_ISR_CMSIS_PRI** 在应用程序中用于作为枚举"QF-aware"中断优先级的偏移量.

"QF aware"的中断：

- 优先级大于等于 **QF_AWARE_ISR_CMSIS_PRI** 
- 可以调用 QF  服务

"QF unaware"的中断：

- 优先级小于 **QF_AWARE_ISR_CMSIS_PRI**
- 不能调用任何 QF 服务

### qv_port.h

```c
#include "qv.h" /* QV platform-independent public interface */
```

## src

### qv.c

**QF_Init** 内部调用了 **QV_INIT**

### qf_actq.c

活动对象队列

### qf_qact.c

活动对象

### qf_qep.c

事件队列

### qf_time.c

时间事件QTimEvt 是静态事件
