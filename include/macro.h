#pragma once

#include <assert.h>
#include <string.h>
#include "util.h"
#include "controllogger.h"

#if defined __GNUC__ || defined __llvm__
/// LIKELY 宏的封装, 告诉编译器优化,条件大概率成立
#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
/// UNLIKELY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif

// 宏是全局定义的 不受命名空间影响
#define VERSION04_ASSERT(x)                                                                                             \
    do                                                                                                                  \
    {                                                                                                                   \
        if (!(x))                                                                                                       \
        {                                                                                                               \
            ULOG_ERROR_SRC("system", "ASSERTION: {}\n backtrace:\n {}", #x, version04::BacktraceToString(100, 2, " ")); \
            assert(x);                                                                                                  \
        }                                                                                                               \
    } while (0)
//
#define VERSION04_ASSERT2(x, w)                                                                                                \
    do                                                                                                                         \
    {                                                                                                                          \
        if (!(x))                                                                                                              \
        {                                                                                                                      \
            ULOG_ERROR_SRC("system", "ASSERTION: {}\n{}\n backtrace:\n {}", #x, w, version04::BacktraceToString(100, 0, " ")); \
            assert(x);                                                                                                         \
        }                                                                                                                      \
    } while (0)
