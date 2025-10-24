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

// 修改目标状态为target
#define Q_TRAN(target_)  \
    ((Q_HSM_UPCAST(me))->temp.fun = Q_STATE_CAST(target_), (QState)Q_RET_TRAN)

// 修改目标状态为super
#define Q_SUPER(super_)  \
    ((Q_HSM_UPCAST(me))->temp.fun = Q_STATE_CAST(super_), (QState)Q_RET_SUPER)
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

### qep_hsm.c

```c
// 保留事件
static QEvt const QEP_reservedEvt_[] = {
    { (QSignal)QEP_EMPTY_SIG_, 0U, 0U },
    { (QSignal)Q_ENTRY_SIG,    0U, 0U },
    { (QSignal)Q_EXIT_SIG,     0U, 0U },
    { (QSignal)Q_INIT_SIG,     0U, 0U }
};

/** 在一个状态转换函数中执行 保留事件动作
 *	QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_)
 *	QEP_TRIG_(t, Q_INIT_SIG)
 */
#define QEP_TRIG_(state_, sig_)	((*(state_))(me, &QEP_reservedEvt_[(sig_)]))

// 状态处理函数执行退出动作Q_EXIT_SIG
#define QEP_EXIT_(state_, qs_id_)	QEP_TRIG_(state_, Q_EXIT_SIG)

// 状态处理函数执行进入动作Q_ENTRY_SIG
#define QEP_ENTER_(state_, qs_id_)	QEP_TRIG_(state_, Q_ENTRY_SIG) 
```

```c
void QHsm_ctor(QHsm * const me, QStateHandler initial) {
     /* QHsm virtual table */
    static struct QHsmVtable const vtable = {
        &QHsm_init_,
        &QHsm_dispatch_
    };
    me->vptr      = &vtable;
    me->state.fun = Q_STATE_CAST(&QHsm_top);
    me->temp.fun  = initial;
}

// 最顶层初始转换 QHsm_top → MyState_Initial → MyState_Active
void QHsm_init_(QHsm * const me, void const * const e)
{
    QStateHandler t = me->state.fun;
    QState r;

    /** @pre the virtual pointer must be initialized, the top-most initial
    * transition must be initialized, and the initial transition must not
    * be taken yet.
    */
    Q_REQUIRE_ID(200, (me->vptr != (struct QHsmVtable *)0)
                      && (me->temp.fun != Q_STATE_CAST(0))
                      && (t == Q_STATE_CAST(&QHsm_top)));

    /* execute the top-most initial tran. */
    r = (*me->temp.fun)(me, Q_EVT_CAST(QEvt));

    /* the top-most initial transition must be taken */
    Q_ASSERT_ID(210, r == (QState)Q_RET_TRAN);

    /* drill down into the state hierarchy with initial transitions... */
    do {
        QStateHandler path[QHSM_MAX_NEST_DEPTH_]; /* tran entry path array */
        int_fast8_t ip = 0; /* tran entry path index */

        path[0] = me->temp.fun;
        (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
        while (me->temp.fun != t) {
            ++ip;
            Q_ASSERT_ID(220, ip < (int_fast8_t)Q_DIM(path));
            path[ip] = me->temp.fun;
            (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
        }
        me->temp.fun = path[0];

        /* retrace the entry path in reverse (desired) order... */
        do {
            QEP_ENTER_(path[ip], qs_id); /* enter path[ip] */
            --ip;
        } while (ip >= 0);

        t = path[0]; /* current state becomes the new source */

        r = QEP_TRIG_(t, Q_INIT_SIG); /* execute initial transition */

    } while (r == (QState)Q_RET_TRAN);

    me->state.fun = t; /* change the current active state */
    me->temp.fun  = t; /* mark the configuration as stable */
}

QState QHsm_top(void const * const me, QEvt const * const e) {
    (void)me;
    (void)e; 
    return (QState)Q_RET_IGNORED; /* the top state ignores all events */
}

void QHsm_dispatch_(QHsm * const me, QEvt const * const e) {
    QStateHandler t = me->state.fun;
    QStateHandler s;
    QState r;

    /** @pre the current state must be initialized and
    * the state configuration must be stable
    */
    Q_REQUIRE_ID(400, (t != Q_STATE_CAST(0))
                       && (t == me->temp.fun));

    /* process the event hierarchically... */
    do {
        s = me->temp.fun;
        r = (*s)(me, e); /* invoke state handler s */

        if (r == (QState)Q_RET_UNHANDLED) { /* unhandled due to a guard? */
            r = QEP_TRIG_(s, QEP_EMPTY_SIG_); /* find superstate of s */
        }
    } while (r == (QState)Q_RET_SUPER);

    /* transition taken? */
    if (r >= (QState)Q_RET_TRAN) {
        QStateHandler path[QHSM_MAX_NEST_DEPTH_];
        int_fast8_t ip;

        path[0] = me->temp.fun; /* save the target of the transition */
        path[1] = t;
        path[2] = s;

        /* exit current state to transition source s... */
        for (; t != s; t = me->temp.fun) {
            if (QEP_TRIG_(t, Q_EXIT_SIG) == (QState)Q_RET_HANDLED) {
                (void)QEP_TRIG_(t, QEP_EMPTY_SIG_); /* find superstate of t */
            }
        }

        ip = QHsm_tran_(me, path);

        /* retrace the entry path in reverse (desired) order... */
        for (; ip >= 0; --ip) {
            QEP_ENTER_(path[ip], qs_id);  /* enter path[ip] */
        }

        t = path[0];      /* stick the target into register */
        me->temp.fun = t; /* update the next state */

        /* drill into the target hierarchy... */
        while (QEP_TRIG_(t, Q_INIT_SIG) == (QState)Q_RET_TRAN) {
            ip = 0;
            path[0] = me->temp.fun;

            (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);/*find superstate */

            while (me->temp.fun != t) {
                ++ip;
                path[ip] = me->temp.fun;
                (void)QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);/* find super */
            }
            me->temp.fun = path[0];

            /* entry path must not overflow */
            Q_ASSERT_ID(410, ip < QHSM_MAX_NEST_DEPTH_);

            /* retrace the entry path in reverse (correct) order... */
            do {
                QEP_ENTER_(path[ip], qs_id); /* enter path[ip] */
                --ip;
            } while (ip >= 0);

            t = path[0]; /* current state becomes the new source */
        }
    }

    me->state.fun = t; /* change the current active state */
    me->temp.fun  = t; /* mark the configuration as stable */
}

// LCA 最近公共祖先
static int_fast8_t QHsm_tran_(QHsm * const me, QStateHandler path[QHSM_MAX_NEST_DEPTH_]) {
    int_fast8_t ip = -1; /* transition entry path index */
    int_fast8_t iq; /* helper transition entry path index */
    QStateHandler t = path[0];
    QStateHandler const s = path[2];
    QState r;

    /* (a) check source==target (transition to self)... */
    if (s == t) {
        QEP_EXIT_(s, qs_id); /* exit the source */
        ip = 0; /* enter the target */
    }
    else {
        (void)QEP_TRIG_(t, QEP_EMPTY_SIG_); /* find superstate of target */

        t = me->temp.fun;

        /* (b) check source==target->super... */
        if (s == t) {
            ip = 0; /* enter the target */
        }
        else {
            (void)QEP_TRIG_(s, QEP_EMPTY_SIG_); /* find superstate of src */

            /* (c) check source->super==target->super... */
            if (me->temp.fun == t) {
                QEP_EXIT_(s, qs_id); /* exit the source */
                ip = 0; /* enter the target */
            }
            else {
                /* (d) check source->super==target... */
                if (me->temp.fun == path[0]) {
                    QEP_EXIT_(s, qs_id); /* exit the source */
                }
                else {
                    /* (e) check rest of source==target->super->super..
                    * and store the entry path along the way
                    */
                    iq = 0; /* indicate that LCA not found */
                    ip = 1; /* enter target and its superstate */
                    path[1] = t;      /* save the superstate of target */
                    t = me->temp.fun; /* save source->super */

                    /* find target->super->super... */
                    r = QEP_TRIG_(path[1], QEP_EMPTY_SIG_);
                    while (r == (QState)Q_RET_SUPER) {
                        ++ip;
                        path[ip] = me->temp.fun; /* store the entry path */
                        if (me->temp.fun == s) { /* is it the source? */
                            iq = 1; /* indicate that LCA found */

                            /* entry path must not overflow */
                            Q_ASSERT_ID(510,
                                ip < QHSM_MAX_NEST_DEPTH_);
                            --ip; /* do not enter the source */
                            r = (QState)Q_RET_HANDLED; /* terminate loop */
                        }
                         /* it is not the source, keep going up */
                        else {
                            r = QEP_TRIG_(me->temp.fun, QEP_EMPTY_SIG_);
                        }
                    }

                    /* the LCA not found yet? */
                    if (iq == 0) {

                        /* entry path must not overflow */
                        Q_ASSERT_ID(520, ip < QHSM_MAX_NEST_DEPTH_);

                        QEP_EXIT_(s, qs_id); /* exit the source */

                        /* (f) check the rest of source->super
                        *                  == target->super->super...
                        */
                        iq = ip;
                        r = (QState)Q_RET_IGNORED; /* LCA NOT found */
                        do {
                            if (t == path[iq]) { /* is this the LCA? */
                                r = (QState)Q_RET_HANDLED; /* LCA found */
                                ip = iq - 1; /* do not enter LCA */
                                iq = -1; /* cause termintion of the loop */
                            }
                            else {
                                --iq; /* try lower superstate of target */
                            }
                        } while (iq >= 0);

                        /* LCA not found? */
                        if (r != (QState)Q_RET_HANDLED) {
                            /* (g) check each source->super->...
                            * for each target->super...
                            */
                            r = (QState)Q_RET_IGNORED; /* keep looping */
                            do {
                                /* exit t unhandled? */
                                if (QEP_TRIG_(t, Q_EXIT_SIG)
                                    == (QState)Q_RET_HANDLED)
                                {

                                    (void)QEP_TRIG_(t, QEP_EMPTY_SIG_);
                                }
                                t = me->temp.fun; /* set to super of t */
                                iq = ip;
                                do {
                                    /* is this LCA? */
                                    if (t == path[iq]) {
                                        /* do not enter LCA */
                                        ip = (int_fast8_t)(iq - 1);
                                        iq = -1; /* break out of inner loop */
                                        /* break out of outer loop */
                                        r = (QState)Q_RET_HANDLED;
                                    }
                                    else {
                                        --iq;
                                    }
                                } while (iq >= 0);
                            } while (r != (QState)Q_RET_HANDLED);
                        }
                    }
                }
            }
        }
    }
    return ip;
}
```

>  状态配置稳定：me->temp.fun == me->state.fun

### qv.c

**QF_Init** 内部调用了 **QV_INIT**

### qf_actq.c

活动对象队列

### qf_qact.c

活动对象

### qf_qep.c

事件队列

### qf_time.c

```c
// 启动定时器
void QTimeEvt_armX(QTimeEvt * const me,
                   QTimeEvtCtr const nTicks, QTimeEvtCtr const interval);
```

时间事件QTimEvt 是静态事件

# NOTE

1. 状态处理函数中初始化 `QState status = Q_HANDLED();`，以防信号处理后未初始化status

2. 订阅（`QActive_subscribe()`）的信号 **只能通过 `QF_PUBLISH()` 投递（发布）**。
    而 **`QACTIVE_POST()` 是直接发送给某一个活动对象的，不走订阅系统**。

3. 静态事件和动态事件(事件池)

   - 静态事件

   ```c
   static QEvt evt = {SIG_LED_TOGGLE, 0, 0};
   QACTIVE_POST(AO_LED, &evt, 0);
   ```

   - 动态事件

   ```c
   QF_PoolInit(poolSto, poolSize, sizeof(MyEvt));
   MyEvt *pe = QF_NEW(MyEvt, SIG_MY_EVENT);
   QACTIVE_POST(AO, &pe->super, 0);// 或 QF_PUBLISH
   ```

   1. ==QTimeEvt_armX 400错误==：定时器**重复启动**导致断言失败 `Q_REQUIRE_ID(400, t->ctr == 0U);`
