/**
 * @file
 * @brief Customizable and memory-efficient assertions for embedded systems
 * @cond
 ******************************************************************************
 * Last updated for version 6.8.2
 * Last updated on  2020-07-07
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
#ifndef QASSERT_H
#define QASSERT_H

/**
 * @note
 * 这个头文件可以用于 C、C++ 以及 C/C++ 混合程序.
 *
 * @note 预处理开关 \b #Q_NASSERT 会禁用断言检查.
 * 但是，通常 __不建议__ 禁用断言, __尤其是__
 * 在生产代码中. 相反，应该仔细设计和测试
 * 断言处理函数 Q_onAssert().
 */

#ifdef Q_NASSERT /* Q_NASSERT 已定义——断言检查被禁用 */

/* 提供空定义, 不会生成任何代码.. */
#define Q_DEFINE_THIS_FILE
#define Q_DEFINE_THIS_MODULE(name_)
#define Q_ASSERT(test_)         ((void)0)
#define Q_ASSERT_ID(id_, test_) ((void)0)
#define Q_ALLEGE(test_)         ((void)(test_))
#define Q_ALLEGE_ID(id_, test_) ((void)(test_))
#define Q_ERROR()               ((void)0)
#define Q_ERROR_ID(id_)         ((void)0)

#else /* Q_NASSERT 未定义——断言检查启用 */

#ifndef QP_VERSION /* 如果 qassert.h 在 QP 外部使用? */

/* 提供 typedef, 以便 qassert.h 可以单独使用... */

/*! 字符串的 typedef */
typedef char char_t;

/*! 断言 ID 和断言行号的 typedef */
typedef int int_t;

#endif

/*! 定义当前文件名 (通过 `__FILE__`) 用于断言. */
/**
 * @brief
 * 这个宏放在每个 C/C++ 模块顶部, 用来定义文件名字符串的单一实例, 供断言报告使用.
 *
 * @note 文件名字符串由标准预处理宏 `__FILE__` 定义.
 * 但是请注意, 不同编译器下, `__FILE__` 可能包含整个路径,
 * 这在日志中可能不方便.
 * @note 该宏后面 __不应__ 加分号.
 */
#define Q_DEFINE_THIS_FILE \
    static char_t const Q_this_module_[] = __FILE__;

/*! 定义用户指定的模块名用于断言. */
/**
 * @brief
 * 这个宏放在每个 C/C++ 模块顶部, 用来定义模块名字符串的单一实例, 供断言报告使用.
 * 与 `__FILE__` 不同, 它允许用户精确控制模块名.
 *
 * @param[in] name_ 模块名称的字符串常量
 *
 * @note 该宏后面 __不应__ 加分号。
 */
#define Q_DEFINE_THIS_MODULE(name_) \
    static char_t const Q_this_module_[] = name_;

/*! 通用断言 */
/**
 * @brief
 * 确保参数 @p test_ 为 TRUE. 如果 @p test_ 为 FALSE,调用 Q_onAssert() 回调函数.
 * 断言位置通过标准宏 `__LINE__` 标识.
 *
 * @param[in] test_ 布尔表达式
 *
 * @note 如果启用了 \b #Q_NASSERT, @p test_ 将 __不会__ 被求值.
 */
#define Q_ASSERT(test_) ((test_)       \
                             ? (void)0 \
                             : Q_onAssert(&Q_this_module_[0], __LINE__))

/*! 带用户指定 ID 的通用断言 */
/**
 * @brief
 * 确保参数 @p test_ 为 TRUE. 如果 @p test_ 为 FALSE, 调用 Q_onAssert() 回调函数.
 * 与 Q_ASSERT 不同, 该宏使用用户提供的 @p id_ 来标识位置, 避免使用行号(因代码插入或
 * 删除可能会改变).
 *
 * @param[in] id_   模块内唯一的断言 ID
 * @param[in] test_ 布尔表达式
 *
 * @note 如果启用了 \b #Q_NASSERT, @p test_ 将 __不会__ 被求值.
 */
#define Q_ASSERT_ID(id_, test_) ((test_)       \
                                     ? (void)0 \
                                     : Q_onAssert(&Q_this_module_[0], (id_)))

/*! 总是会执行 @p test_ 的断言 */
/**
 * @brief
 * 与 Q_ASSERT() 类似, 但即使启用了 \b #Q_NASSERT,
 * @p test_ 表达式也会被求值.
 * 但是, 如果定义了 \b #Q_NASSERT, 即使 @p test_ 为 FALSE,
 * Q_onAssert() 也 __不会__ 被调用.
 *
 * @param[in] test_ 布尔表达式（__始终__求值）
 */
#define Q_ALLEGE(test_)         Q_ASSERT(test_)

/*! 带用户指定 ID 的"总是求值"断言 */
/**
 * @brief
 * 与 Q_ASSERT_ID() 类似, 但即使启用了 \b #Q_NASSERT,
 * @p test_ 表达式也会被求值.
 * 但是, 如果定义了 \b #Q_NASSERT, 即使 @p test_ 为 FALSE,
 * Q_onAssert() 也 __不会__ 被调用.
 *
 * @param[in] id_   模块内唯一的断言 ID
 * @param[in] test_ 布尔表达式
 */
#define Q_ALLEGE_ID(id_, test_) Q_ASSERT_ID((id_), (test_))

/*! 表示错误路径的断言 */
/**
 * @brief
 * 如果执行到该断言, 就调用 Q_onAssert().
 *
 * @note 如果启用了 \b #Q_NASSERT, 则不会做任何事.
 */
#define Q_ERROR() \
    Q_onAssert(&Q_this_module_[0], __LINE__)

/*! 带用户指定 ID 的错误路径断言 */
/**
 * @brief
 * 如果执行到该断言, 就调用 Q_onAssert().
 * 与 Q_ERROR 不同, 该宏使用用户提供的 @p id_ 来标识断言位置,
 * 避免行号变化问题.
 *
 * @param[in] id_ 模块内唯一的断言 ID
 *
 * @note 如果启用了 \b #Q_NASSERT, 则不会做任何事.
 */
#define Q_ERROR_ID(id_) \
    Q_onAssert(&Q_this_module_[0], (id_))

#endif /* Q_NASSERT */

/****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Q_NORETURN
/*! 不返回的函数限定符 */
#define Q_NORETURN void
#endif /*  Q_NORETURN */

/*! 断言失败时调用的回调函数 */
/**
 * @brief
 * 这是一个由应用程序定义的回调函数.
 * 它需要在应用中实现, 用于执行系统安全关机或复位.
 *
 * @param[in] module  发生断言失败的模块/文件名 (零结尾 C 字符串常量)
 * @param[in] location 断言位置, 可以是行号, 也可以是用户指定 ID
 *
 * @note 该函数不应返回, 因为断言失败后继续执行没有意义.
 *
 * @note Q_onAssert() 是系统故障时的最后防线.
 * 它的实现应该经过 __非常仔细__ 的设计和测试,
 * 包括但不限于栈溢出、栈破坏、或从中断调用 Q_onAssert() 等情况.
 *
 * @note 通常 __不建议__ 把 Q_onAssert() 实现成死循环,
 * 这样会占用 CPU. 在调试时, Q_onAssert() 是打断点的理想位置.
 *
 * 被以下宏调用: #Q_ASSERT, #Q_REQUIRE, #Q_ENSURE,
 * #Q_ERROR, #Q_ALLEGE, #Q_ASSERT_ID, #Q_REQUIRE_ID,
 * #Q_ENSURE_ID, #Q_ERROR_ID, 和 #Q_ALLEGE_ID.
 */
Q_NORETURN Q_onAssert(char_t const *const module, int_t const location);

#ifdef __cplusplus
}
#endif

/*! 检查前置条件的断言 */
/**
 * @brief
 * 等价于 \b #Q_ASSERT, 只是宏名更好地表明其意图.
 *
 * @param[in] test_ 布尔表达式
 */
#define Q_REQUIRE(test_) Q_ASSERT(test_)

/*! 带 ID 的前置条件断言 */
/**
 * @brief
 * 等价于 \b #Q_ASSERT_ID, 只是宏名更好地表明其意图.
 *
 * @param[in] id_   模块内唯一的断言 ID
 * @param[in] test_ 布尔表达式
 */
#define Q_REQUIRE_ID(id_, test_) Q_ASSERT_ID((id_), (test_))

/*! 检查后置条件的断言 */
/**
 * 等价于 \b #Q_ASSERT, 只是宏名更好地表明其意图.
 *
 * @param[in] test_ 布尔表达式
 */
#define Q_ENSURE(test_) Q_ASSERT(test_)

/*! 带 ID 的后置条件断言 */
/**
 * @brief
 * 等价于 \b #Q_ASSERT_ID, 只是宏名更好地表明其意图.
 *
 * @param[in] id_   模块内唯一的断言 ID
 * @param[in] test_ 布尔表达式
 */
#define Q_ENSURE_ID(id_, test_) Q_ASSERT_ID((id_), (test_))

/*! 检查不变式的断言 */
/**
 * @brief
 * 等价于 \b #Q_ASSERT, 只是宏名更好地表明其意图.
 *
 * @param[in] test_ 布尔表达式
 */
#define Q_INVARIANT(test_) Q_ASSERT(test_)

/*! 带 ID 的不变式断言 */
/**
 * @brief
 * 等价于 \b #Q_ASSERT_ID, 只是宏名更好地表明其意图.
 *
 * @param[in] id_   模块内唯一的断言 ID
 * @param[in] test_ 布尔表达式
 */
#define Q_INVARIANT_ID(id_, test_) Q_ASSERT_ID((id_), (test_))

/*! 静态(编译期)断言 */
/**
 * @brief
 * 如果 @p test_ 为 FALSE, 会故意在编译时报错.
 * 该宏利用了 C/C++ 中数组维度不能为负的特性.
 * 这种断言在编译期检查, 没有运行时开销.
 *
 * @param[in] test_ 编译期布尔表达式
 */
#define Q_ASSERT_STATIC(test_) \
    extern int_t Q_assert_static[(test_) ? 1 : -1]

#define Q_ASSERT_COMPILE(test_) Q_ASSERT_STATIC(test_)

/*! 辅助宏, 用于计算一维数组 @p array_ 的大小 */
#define Q_DIM(array_) (sizeof(array_) / sizeof((array_)[0U]))

#endif /* QASSERT_H */
