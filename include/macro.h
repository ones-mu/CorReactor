#pragma once

#include <assert.h>
#include <string.h>
#include "util.h"
#include "controllogger.h"

// 宏是全局定义的 不受命名空间影响
#define VERSION04_ASSERT(x) \
    do { \
        if (!(x)) { \
            ULOG_ERROR("system", "ASSERTION: {}\n backtrace:\n {}", #x, version04::BacktraceToString(100, 2, " ")); \
            assert(x); \
        } \
    } while (0)
//
#define VERSION04_ASSERT2(x, w) \
    do { \
        if (!(x)) { \
            ULOG_ERROR("system", "ASSERTION: {}\n{}\n backtrace:\n {}", #x, w, version04::BacktraceToString(100, 0, " ")); \
            assert(x); \
        } \
    } while (0)
