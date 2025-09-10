/**
 * @file
 * @brief Public QEP/C interface
 * @ingroup qep
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
#ifndef QEP_H
#define QEP_H

/****************************************************************************/
/*! 当前 QP 版本号(十进制常量 XXYZ),
 * 其中 XX 为 2 位主版本号, Y 为 1 位次版本号, Z 为 1 位修订号.
 */
#define QP_VERSION 693U

/*! 当前 QP 版本号字符串, 格式为 XX.Y.Z,
 * 其中 XX 为 2 位主版本号, Y 为 1 位次版本号, Z 为 1 位修订号.
 */
#define QP_VERSION_STR "6.9.3"

/*! 加密后的当前 QP 发行版 (6.9.3) 及发布日期 (2021-04-12) */
#define QP_RELEASE 0x8295AA8AU

/****************************************************************************/
/* 基本数值类型的 typedef */

/*! 字符串的 typedef */
/**
 * @brief
 * 该 typedef 指定了专门用于字符串的字符类型.
 * 使用此类型而不是普通的 'char'
 */
typedef char char_t;

/*! 断言中的行号以及 QF_run() 返回值的 typedef */
typedef int int_t;

/*! 用于表达式中的无符号 int 提升的 typedef */
typedef unsigned uint_t;

/*! 用于事件信号的枚举类型的 typedef */
typedef int enum_t;

/*! IEEE 754 32 位浮点数 */
/**
 * @note QP 在内部实现中不会使用浮点数类型,
 * 唯一的例外是 QS 软件追踪(tracing),
 * 其中提供了输出浮点数的工具, 供应用层的追踪记录使用.
 */
typedef float float32_t;

/*! IEEE 754 64 位浮点数 */
/**
 * @note QP 在内部实现中不会使用浮点数类型,
 * 唯一的例外是 QS 软件追踪(tracing),
 * 其中提供了输出浮点数的工具, 供应用层的追踪记录使用.
 */
typedef double float64_t;

/*! 当前 QP 版本号字符串, 存放在 ROM 中, 基于 QP_VERSION_STR */
extern char_t const QP_versionStr[7];

/****************************************************************************/
#ifndef Q_SIGNAL_SIZE

/*! 事件信号的大小(以字节为单位) 有效值: 1U、2U 或 4U; 默认值为 2U */
/**
 * @brief
 * 该宏可以在 QEP 移植文件 (qep_port.h) 中定义,
 * 用于配置 ::QSignal 类型. 如果该宏没有定义,
 * 默认使用 2 字节.
 */
#define Q_SIGNAL_SIZE 2U
#endif
#if (Q_SIGNAL_SIZE == 1U)
typedef uint8_t QSignal;
#elif (Q_SIGNAL_SIZE == 2U)
/*! QSignal 表示事件的信号. */
/**
 * @brief
 * 事件与信号之间的关系如下:
 * 在 UML 中, 信号是异步刺激的规范, 它触发反应, 因此信号是事件的核心组成部分.
 * (信号传达的是"发生了什么"的类型信息) 但是, 事件还可以包含额外的定量信息,
 * 以事件参数的形式描述发生情况的细节.
 */
typedef uint16_t QSignal;
#elif (Q_SIGNAL_SIZE == 4U)
typedef uint32_t QSignal;
#else
#error "Q_SIGNAL_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

/****************************************************************************/
/*! 事件类 */
/**
 * @brief
 * QEvt 表示无参数事件, 并作为带参数事件派生的基结构体.
 */
typedef struct {
    QSignal sig;              /*!< 事件实例的信号 */
    uint8_t poolId_;          /*!< 所属事件池 ID（静态事件为 0）*/
    uint8_t volatile refCtr_; /*!< 引用计数器 */
} QEvt;

#ifdef Q_EVT_CTOR /* 是否提供 QEvt 类的构造函数? */

/*! 自定义事件构造函数
 * @public @memberof QEvt
 */
QEvt *QEvt_ctor(QEvt *const me, enum_t const sig);

#endif

/****************************************************************************/
/*! 将 ::QHsm 的子类指针向上转换为基类 ::QHsm 指针 */
/**
 * @brief
 * 在面向对象编程中, 从子类向父类的上行转换(upcast)是非常频繁且 __安全__ 的操作,
 * 例如 C++ 语言会自动完成这种转换.
 * 但是在 C 语言中, OOP 是通过一系列编码约定实现的, 编译器并不知道某些类型之间存在继承关系.
 * 因此在 C 中必须显式地进行上行转换.
 */
#define Q_HSM_UPCAST(ptr_) ((QHsm *)(ptr_))

/*! 将事件指针向下转换为 QEvt 的子类 @p class_ */
/**
 * @brief
 * QEvt 表示无参数事件, 是带参数事件派生的基结构体.
 * 这个宏封装了 QEvt 指针的向下转换(downcast)
 */
#define Q_EVT_CAST(class_) ((class_ const *)e)

/*! 将无符号整型指针 @p uintptr_ 转换为类型 @p type_ 的指针 */
/**
 * @brief
 * 这个宏封装了向 (type_ *) 的类型转换, QP 移植层或应用层可能用它访问嵌入式硬件寄存器
 */
#define Q_UINT2PTR_CAST(type_, uintptr_) ((type_ *)(uintptr_))

/****************************************************************************/
/*! 状态机/动作处理函数的返回类型 typedef */
typedef uint_fast8_t QState;

/* 前向声明 */
typedef struct QXThread QXThread;

/*! 状态处理函数指针 typedef  */
typedef QState (*QStateHandler)(void *const me, QEvt const *const e);

/*! 动作处理函数指针 typedef */
typedef QState (*QActionHandler)(void *const me);

/*! 线程处理函数指针 typedef */
typedef void (*QXThreadHandler)(QXThread *const me);

/*! 将函数指针转换为 ::QStateHandler 类型 */
/**
 * @brief
 * 该宏封装了特定状态处理函数指针向 ::QStateHandler 的转换
 */
#define Q_STATE_CAST(handler_) ((QStateHandler)(handler_))

/*! 将函数指针转换为 QActionHandler 类型 */
/**
 * @brief
 * 该宏封装了特定动作处理函数指针向 ::QActionHandler 的转换
 */
#define Q_ACTION_CAST(action_) ((QActionHandler)(action_))

/* 前向声明 */
struct QMState;
struct QHsmVtable;
typedef struct QMTranActTable QMTranActTable;

/*! ::QHsm 类(层次状态机)的属性 */
/**
 * @brief
 * 该联合体表示存储在 ::QHsm 类的 'state' 和 'temp' 属性中的可能值
 */
union QHsmAttr {
    QStateHandler fun;           /*!< 指向状态处理函数的指针 */
    QActionHandler act;          /*!< 指向动作处理函数的指针 */
    QXThreadHandler thr;         /*!< 指向线程处理函数的指针 */
    struct QMState const *obj;   /*!< 指向 QMState 对象的指针 */
    QMTranActTable const *tatbl; /*!< 转移-动作表指针 */
};

/****************************************************************************/
/*! 层次状态机类 (Hierarchical State Machine, HSM) */
/**
 * @brief
 * ::QHsm 表示一个层次状态机, 支持:
 * - 状态的层次嵌套
 * - 进入/退出动作 (entry/exit actions)
 * - 初始转换 (initial transitions)
 * - 任意复合状态下的历史状态转换 (transitions to history)
 * 该类设计用于在 C 语言中手动编写 HSM, 同时也被 QM 建模工具支持.
 *
 * @n
 * ::QHsm 也是 ::QMsm 状态机的基类, QMsm 提供更高的效率, 但需要使用 QM 建模工具生成代码.
 *
 * @note ::QHsm 不打算被直接实例化, 而是作为应用代码中状态机派生的基结构体.
 */
typedef struct {
    struct QHsmVtable const *vptr; /*!< 虚函数表指针 */
    union QHsmAttr state;          /*!< 当前活动状态(状态变量) */
    union QHsmAttr temp;           /*!< 临时变量: 用于转换链, 目标状态等 */
} QHsm;

/*! ::QHsm 类的虚函数表 */
struct QHsmVtable {
#ifdef Q_SPY
    /*! 触发 HSM 的最顶层初始转换 */
    void (*init)(QHsm *const me, void const *const e,
                 uint_fast8_t const qs_id);

    /*! 向 HSM 分发事件 */
    void (*dispatch)(QHsm *const me, QEvt const *const e,
                     uint_fast8_t const qs_id);

    /*! 获取 HSM 的当前状态处理函数 */
    QStateHandler (*getStateHandler)(QHsm *const me);
#else
    void (*init)(QHsm *const me, void const *const e);
    void (*dispatch)(QHsm *const me, QEvt const *const e);
#endif /* Q_SPY */
};

/* QHsm public operations... */

#ifdef Q_SPY
/*! 多态执行 HSM 的最顶层初始转换
 * @public @memberof QHsm
 */
/**
 * @param[in,out] me_  指针
 * @param[in]     e_   常量指针, 指向 ::QEvt 或其派生类
 * @param[in]     qs_id_ QS 本地过滤 ID
 *
 * @note 只能在状态机"构造函数"之后调用一次.
 */
#define QHSM_INIT(me_, par_, qs_id_)                   \
    do {                                               \
        Q_ASSERT((me_)->vptr);                         \
        (*(me_)->vptr->init)((me_), (par_), (qs_id_)); \
    } while (false)

/*! ::QHsm 子类的最顶层初始转换实现 */
void QHsm_init_(QHsm *const me, void const *const e,
                uint_fast8_t const qs_id);
#else

#define QHSM_INIT(me_, par_, dummy)          \
    do {                                     \
        Q_ASSERT((me_)->vptr);               \
        (*(me_)->vptr->init)((me_), (par_)); \
    } while (false)

/*! ::QHsm 子类的最顶层初始转换实现 */
void QHsm_init_(QHsm *const me, void const *const e);

#endif /* Q_SPY */

#ifdef Q_SPY
/*! 多态地向 HSM 分发事件
 * @public @memberof QHsm
 */
/**
 * @brief
 * 以 Run-to-Completion 方式一次处理一个事件.
 *
 * @param[in,out] me_ 指针
 * @param[in]     e_  常量指针, 指向 ::QEvt 或其派生结构体
 * @note 必须在"构造函数"之后, 并在 QHSM_INIT() 调用之后使用.
 */
#define QHSM_DISPATCH(me_, e_, qs_id_) \
    ((*(me_)->vptr->dispatch)((me_), (e_), (qs_id_)))

/*! 向 ::QHsm 子类分发事件的实现 */
void QHsm_dispatch_(QHsm *const me, QEvt const *const e,
                    uint_fast8_t const qs_id);

/*! 获取 ::QHsm 子类的状态处理函数实现 */
QStateHandler QHsm_getStateHandler_(QHsm *const me);
#else

#define QHSM_DISPATCH(me_, e_, dummy) \
    ((*(me_)->vptr->dispatch)((me_), (e_)))

/*! 向 ::QHsm 子类分发事件的实现 */
void QHsm_dispatch_(QHsm *const me, QEvt const *const e);

#endif /* Q_SPY */

/*! 获取 HSM 当前活动状态(只读)
 * @public @memberof QHsm
 */
/**
 * @param[in] me_ 指针
 * @returns 返回 ::QHsm 子类的当前活动状态处理函数
 * @note 该宏在 QM 中用于自动生成状态历史记录代码
 */
#define QHsm_state(me_) (Q_STATE_CAST(Q_HSM_UPCAST(me_)->state.fun))

/*! 获取指定父状态的当前活动子状态
 * @public @memberof QHsm
 */
/**
 * @param[in] me_     指针
 * @param[in] parent_ 指向父状态处理函数的指针
 * @returns 返回指定父状态的当前活动子状态处理函数
 * @note 该宏在 QM 中用于自动生成状态历史记录代码
 */
#define QHsm_childState(me_, parent_) \
    QHsm_childState_(Q_HSM_UPCAST(me_), Q_STATE_CAST(parent_))

/*! 检查给定状态是否属于 ::QHsm 子类当前活动状态配置
 * @public @memberof QHsm
 */
bool QHsm_isIn(QHsm *const me, QStateHandler const state);

/* QHsm 受保护操作 */
/*! ::QHsm 的受保护"构造函数"
 * @protected @memberof QHsm
 */
void QHsm_ctor(QHsm *const me, QStateHandler initial);

/*! 顶层状态
 * @protected @memberof QHsm
 */
QState QHsm_top(void const *const me, QEvt const *const e);

/* QHsm 私有操作 */
/*! 获取父状态的当前活动子状态的辅助函数
 * @private @memberof QHsm
 */
QStateHandler QHsm_childState_(QHsm *const me, QStateHandler const parent);

/****************************************************************************/
/*! QM 状态机实现策略
 * @extends QHsm
 */
/**
 * @brief
 * ::QMsm (QM 状态机) 提供比 ::QHsm 更高效的状态机实现策略,
 * 但需要使用 QM 建模工具生成代码. QMsm 的运行速度最快,
 * 所需的运行时支持最少（事件处理器占用的代码空间最小）.
 *
 * @note
 * ::QMsm 不打算被直接实例化, 而是作为应用代码中状态机派生的基结构体.
 */
typedef struct {
    QHsm super; /*!< 继承 ::QHsm  */
} QMsm;

/*! ::QMsm 类的状态对象 */
/**
 * @brief
 * 该类用于组合 ::QMsm 状态的属性, 包括:
 * - 父状态(状态嵌套)
 * - 关联的状态处理函数
 * - 退出动作处理函数
 *
 * 这些属性在 QMsm_dispatch() 和 QMsm_init() 函数中使用.
 *
 * @attention
 * ::QMState 类仅供 QM 代码生成器使用, 不应在手工编写的代码中使用.
 */
struct QMState {
    struct QMState const *superstate; /*!< 该状态的父状态 */
    QStateHandler const stateHandler; /*!< 状态处理函数 */
    QActionHandler const entryAction; /*!< 进入动作处理函数 */
    QActionHandler const exitAction;  /*!< 退出动作处理函数 */
    QActionHandler const initAction;  /*!< 初始化动作处理函数 */
};
typedef struct QMState QMState;

/*! 元状态机的转换-动作表 */
struct QMTranActTable {
    struct QMState const *target; /*!< 目标状态 */
    QActionHandler const act[1];  /*!< 动作处理函数数组 */
};

/* QMsm 公共操作 */

/*! 获取 MSM 的当前活动状态 (只读)
 * @public @memberof QMsm
 */
/**
 * @param[in] me_ 指针
 * @returns 当前活动状态对象
 * @note 该宏在 QM 中用于自动生成状态历史记录代码
 */
#define QMsm_stateObj(me_) (Q_HSM_UPCAST(me_)->state.obj)

/*! 获取指定父状态的当前活动子状态(::QMsm)
 * @public @memberof QMsm
 */
/**
 * @param[in] me_     指针
 * @param[in] parent_ 指向父状态对象的指针
 * @returns 返回指定父状态的当前活动子状态对象
 * @note 该宏在 QM 中用于自动生成状态历史记录代码
 */
#define QMsm_childStateObj(me_, parent_) \
    QMsm_childStateObj_(Q_HSM_UPCAST(me_), (parent_))

/*! 获取指定父状态的当前活动子状态的辅助函数
 * @public @memberof QMsm
 */
QMState const *QMsm_childStateObj_(QMsm const *const me,
                                   QMState const *const parent);

/*! 检查给定状态是否属于 MSM 当前活动状态配置
 * @public @memberof QMsm
 */
bool QMsm_isInState(QMsm const *const me, QMState const *const state);

/* QMsm 受保护操作 */

/*! ::QMsm 构造函数
 * @protected @memberof QMsm
 */
void QMsm_ctor(QMsm *const me, QStateHandler initial);

/* QMsm 私有操作 */

/*! ::QMsm 的最顶层初始转换实现
 * @private @memberof QMsm
 */
#ifdef Q_SPY
void QMsm_init_(QHsm *const me, void const *const e,
                uint_fast8_t const qs_id);
#else
void QMsm_init_(QHsm *const me, void const *const e);
#endif

/*! 向 ::QMsm 分发事件的实现
 * @private @memberof QMsm
 */
#ifdef Q_SPY
void QMsm_dispatch_(QHsm *const me, QEvt const *const e,
                    uint_fast8_t const qs_id);
#else
void QMsm_dispatch_(QHsm *const me, QEvt const *const e);
#endif

/*! 获取 ::QMsm 中的状态处理函数的实现
 * @private @memberof QMsm
 */
#ifdef Q_SPY
QStateHandler QMsm_getStateHandler_(QHsm *const me);
#endif

/*! 在状态处理函数中执行普通转换或初始转换时调用的宏, 仅适用于 ::QHsm 子类 */
#define Q_TRAN(target_) \
    ((Q_HSM_UPCAST(me))->temp.fun = Q_STATE_CAST(target_), (QState)Q_RET_TRAN)

/*! 在状态处理函数中执行历史状态转换时调用的宏,  仅适用于分层状态机(HSM) */
#define Q_TRAN_HIST(hist_) \
    ((Q_HSM_UPCAST(me))->temp.fun = (hist_), (QState)Q_RET_TRAN_HIST)

/*! 在状态处理函数中指定某个状态的超状态时调用的宏.  仅适用于 ::QHsm 子类 */
#define Q_SUPER(super_) \
    ((Q_HSM_UPCAST(me))->temp.fun = Q_STATE_CAST(super_), (QState)Q_RET_SUPER)

/*! 在状态处理函数中，当一个事件被成功处理时调用的宏, 适用于 HSM 和 FSM */
#define Q_HANDLED() ((QState)Q_RET_HANDLED)

/*! 在状态处理函数中，当尝试处理事件但guard条件计算结果为 false,
 * 且没有其他显式的事件处理方式时调用的宏.
 * 仅适用于 ::QHsm 子类。
 */
#define Q_UNHANDLED() ((QState)Q_RET_UNHANDLED)

/*! 提供严格类型化的"空动作(zero-action)"，
 * 用于在转换-动作表(transition-action-table)的动作列表中作为结束标记.
 */
#define Q_ACTION_NULL ((QActionHandler)0)

/****************************************************************************/
/*! 状态/动作处理函数可能返回的所有取值 */
/**
 * @note 枚举值的顺序对算法正确性有影响
 */
enum {
    /* 未处理, 需要"bubble up"传递 */
    Q_RET_SUPER,     /*!< 事件传递给超状态处理 */
    Q_RET_SUPER_SUB, /*!< 事件传递给子状态机的超状态处理 */
    Q_RET_UNHANDLED, /*!< 由于守卫条件不满足, 事件未处理 */

    /* 已处理, 不需要"bubble up" */
    Q_RET_HANDLED, /*!< 事件已处理(内部转换) */
    Q_RET_IGNORED, /*!< 事件被静默忽略(bubble up to top) */

    /* 进入/退出 */
    Q_RET_ENTRY, /*!< 执行了状态进入动作 */
    Q_RET_EXIT,  /*!< 执行了状态退出动作 */

    /* 无副影响 */
    Q_RET_NULL, /*!< 返回值没有任何效果 */

    /* 需要在 ::QMsm 中执行转换-动作表 */
    Q_RET_TRAN,      /*!< 普通状态转换 */
    Q_RET_TRAN_INIT, /*!< 状态或子状态机中的初始转换 */
    Q_RET_TRAN_EP,   /*!< 子状态机入口点转换 */

    /* 转换时会额外修改 me->state */
    Q_RET_TRAN_HIST, /*!< 转换到给定状态的历史状态 */
    Q_RET_TRAN_XP    /*!< 子状态机出口点转换 */
};

#ifdef Q_SPY
/*! 在 QM 动作处理函数中执行进入动作时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_ENTRY(state_) \
    ((Q_HSM_UPCAST(me))->temp.obj = (state_), (QState)Q_RET_ENTRY)

/*! 在 QM 动作处理函数中执行退出动作时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_EXIT(state_) \
    ((Q_HSM_UPCAST(me))->temp.obj = (state_), (QState)Q_RET_EXIT)

#else
#define QM_ENTRY(dummy) ((QState)Q_RET_ENTRY)

#define QM_EXIT(dummy)  ((QState)Q_RET_EXIT)

#endif /* Q_SPY */

/*! 在 QM 子状态机退出处理函数中调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_SM_EXIT(state_) \
    ((Q_HSM_UPCAST(me))->temp.obj = (state_), (QState)Q_RET_EXIT)

/*! 在 QM 状态处理函数中执行普通状态转换时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_TRAN(tatbl_) ((Q_HSM_UPCAST(me))->temp.tatbl = (QMTranActTable *)(tatbl_), (QState)Q_RET_TRAN)

/*! 在 QM 状态处理函数中执行初始转换时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_TRAN_INIT(tatbl_) ((Q_HSM_UPCAST(me))->temp.tatbl = (QMTranActTable *)(tatbl_), (QState)Q_RET_TRAN_INIT)

/*! 在 QM 状态处理函数中执行历史状态转换时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_TRAN_HIST(history_, tatbl_)                                \
    ((((Q_HSM_UPCAST(me))->state.obj = (history_)),                   \
      ((Q_HSM_UPCAST(me))->temp.tatbl = (QMTranActTable *)(tatbl_))), \
     (QState)Q_RET_TRAN_HIST)

/*! 在 QM 状态处理函数中执行进入子状态机入口点的转换时调用的宏 */
#define QM_TRAN_EP(tatbl_) ((Q_HSM_UPCAST(me))->temp.tatbl = (struct QMTranActTable *)(tatbl_), (QState)Q_RET_TRAN_EP)

/*! 在 QM 状态处理函数中执行子状态机出口点转换时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_TRAN_XP(xp_, tatbl_)                                       \
    ((((Q_HSM_UPCAST(me))->state.act = (xp_)),                        \
      ((Q_HSM_UPCAST(me))->temp.tatbl = (QMTranActTable *)(tatbl_))), \
     (QState)Q_RET_TRAN_XP)

/*! 在 QM 状态处理函数中，当事件被处理时调用的宏. 仅适用于 ::QMsm 子类 */
#define QM_HANDLED() ((QState)Q_RET_HANDLED)

/*! 在 QM 状态处理函数中, 当尝试处理事件但守卫条件为 false, 且没有其他显式的事件处理方式时调用的宏
 * 仅适用于 ::QMsm 子类。
 */
#define QM_UNHANDLED() ((QState)Q_RET_UNHANDLED)

/*! 在 QM 状态处理函数中，当需要指定由超状态处理事件时调用的宏. 仅适用于 ::QMsm */
#define QM_SUPER() ((QState)Q_RET_SUPER)

/*! 在 QM 子状态机处理函数中, 当需要指定由宿主状态处理事件时调用的宏. 仅适用于 ::QMsm */
#define QM_SUPER_SUB(host_) \
    ((Q_HSM_UPCAST(me))->temp.obj = (host_), (QState)Q_RET_SUPER_SUB)

/*! 提供严格类型化的空状态(zero-state), 用于子状态机。仅适用于 QP::QMsm 的子类 */
#define QM_STATE_NULL ((QMState *)0)

/*! QEP 保留信号 */
enum {
    Q_ENTRY_SIG = 1, /*!< 用于编码进入动作的信号 */
    Q_EXIT_SIG,      /*!< 用于编码退出动作的信号 */
    Q_INIT_SIG,      /*!< 用于编码初始转换的信号 */
    Q_USER_SIG       /*!< 用户可用的第一个信号 */
};

#endif /* QEP_H */
