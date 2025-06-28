#include "iomanager.h"
#include <sys/epoll.h>
#include <unistd.h>

namespace version04
{
    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) : Scheduler(threads, use_caller, name)
    {
        m_epfd=epoll_create(1);
        VERSION04_ASSERT2(m_epfd > 0, "IOManager::IOManager epoll_create failed");
    }
}