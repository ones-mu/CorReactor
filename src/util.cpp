#include <execinfo.h>
#include <sys/time.h>
#include "util.h"
#include "controllogger.h"
#include "fiber.h"
namespace version04
{
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid); // 获取的是子线程的Id而不是进程的Id 完全等价于gettid()
    }

    uint64_t GetFiberId()
    {
        return version04::Fiber::GetFiberId();
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **buffer = (void **)malloc(sizeof(void *) * size);
        int ntprs;
        ntprs = ::backtrace(buffer, size);
        char **strings = backtrace_symbols(buffer, ntprs);
        if (strings == nullptr)
        {
            ULOG_ERROR_SRC("system", "backtrace_symbols error");
            return;
        }
        for (int i = skip; i < ntprs; i++)
        {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(buffer);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (auto &s : bt)
        {
            ss << prefix << s << std::endl;
        }
        return ss.str();
    }

    int evaluate_expression(const std::string &expr)
    {
        size_t pos = expr.find('*');
        if (pos != std::string::npos)
        {
            int a = std::stoi(expr.substr(0, pos));
            int b = std::stoi(expr.substr(pos + 1));
            return a * b;
        }
        else
        {
            return std::stoi(expr); // 如果不是表达式，直接转换
        }
    }
    uint64_t GetCurrentMS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }

    std::string Time2Str(time_t ts, const std::string &format)
    {
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return buf;
    }
}
