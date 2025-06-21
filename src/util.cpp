#include "util.h"
#include <execinfo.h>
#include "controllogger.h"
namespace version04
{
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid); // 获取的是子线程的Id而不是进程的Id 完全等价于gettid()
    }

    uint32_t GetFiberId()
    {
        return 0;
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void** buffer=(void**) malloc(sizeof(void*)*size);
        int ntprs;
        ntprs=::backtrace(buffer,size);
        char** strings=backtrace_symbols(buffer,ntprs);
        if(strings==nullptr)
        {
            ULOG_ERROR("system","backtrace_symbols error");
            return;
        }
        for(int i=skip;i<ntprs;i++)
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
        for(auto& s : bt)
        {
            ss<<prefix<<s<<std::endl;
        }
        return ss.str();
    }
}
