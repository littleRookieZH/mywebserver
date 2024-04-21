/**
 * 常用函数宏的封装
*/
#include <assert.h>
#include "../log_module/log.h"
#include "./util.h"

#ifndef __WEBS_MACRO_H__
#define __WEBS_MACRO_H__

#if defined __GNUC__ || defined __llvm__
/* LIKCLY 宏的封装，告诉编译器优化，条件大概率成立；使用两次取反的目的是：保证一个确定的bool值 */
#define WEBS_LIKELY(x) __builtin_expect(!!(x), 1)

/* LIKCLY 宏的封装，告诉编译器优化，条件大概率不成立 */
#define WEBS_UNLIKELY(x) __builtin_expect(!!(x), 0)

#else
#define WEBS_LIKELY(x) (x)
#define WEBS_UNLIKELY(x) (x)
#endif

/* 
SYLAR_UNLIKELY(x)：期望x是假，如果x确实为假，则相当于SYLAR_UNLIKELY(x)为假
如果x为真，则相当于SYLAR_UNLIKELY(x)为真
!(x)就是将上面的条件取反

可以理解为__builtin_expect(!!(x), 1)并不会影响条件判断结果，如果x为真，__builtin_expect(!!(x), 1)后依旧为真
__builtin_expect的作用是优化编译器并不是改变条件判断的值
*/

/// 断言宏封装
#define WEBS_ASSERT(x)                                                               \
    if (WEBS_UNLIKELY(!(x))) {                                                       \
        WEBS_LOG_ERROR(WEBS_LOG_ROOT()) << "ASSERTION :" << #x << "\nbacktrace:\n"   \
                                        << webs::BacktraceToString(100, 2, "     "); \
        assert(x);                                                                   \
    }

/// 断言宏封装
#define WEBS_ASSERT2(x, w)                              \
    if (WEBS_UNLIKELY(!(x))) {                          \
        WEBS_LOG_ERROR(WEBS_LOG_ROOT())                 \
            << "ASSERTION : " #x << "\n"                \
            << w << "\n"                                \
            << "backtrace : \n"                         \
            << webs::BacktraceToString(100, 2, "    "); \
        assert(x);                                      \
    }

#endif
