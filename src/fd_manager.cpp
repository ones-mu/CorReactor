#include "fd_manager.h"
#include "hook.h"
// #include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

namespace version04
{
    FdCtx::FdCtx(int fd)
        : m_isInit(false), m_isSocket(false), m_sysNonblock(false), m_userNonblock(false), m_isClosed(false), m_fd(fd), m_recvTimeout(-1), m_sendTimeout(-1)
    {
        init();
    }

    FdCtx::~FdCtx()
    {
    }

    bool FdCtx::init()
    {
        // 初始化过了
        if (m_isInit)
        {
            return true;
        }
        m_recvTimeout = -1;
        m_sendTimeout = -1;

        struct stat fd_stat;
        // 获取文件状态信息的函数 成功时返回0，失败时返回-1，并设置errno
        if (-1 == fstat(m_fd, &fd_stat))
        {
            m_isInit = false;
            m_isSocket = false;
        }
        else
        {
            m_isInit = true;
            // 判断文件是否是socket
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }
        // 将socket设置为非阻塞模式
        if (m_isSocket)
        {
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            if (!(flags & O_NONBLOCK))
            {
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
            // hook是否非阻塞
            m_sysNonblock = true;
        }
        else
        {
            m_sysNonblock = false;
        }

        m_userNonblock = false;
        m_isClosed = false;
        return m_isInit;
    }
    // 设置超时事件
    void FdCtx::setTimeout(int type, uint64_t v)
    {
        if (type == SO_RCVTIMEO)
        {
            // 读超时时间毫秒
            m_recvTimeout = v;
        }
        else
        {
            // 写超时时间毫秒
            m_sendTimeout = v;
        }
    }
    // 获得超时事件
    uint64_t FdCtx::getTimeout(int type)
    {
        if (type == SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        else
        {
            return m_sendTimeout;
        }
    }

    FdManager::FdManager()
    {
        m_datas.resize(64);
    }
    // 获取/创建文件句柄类FdCtx
    FdCtx::ptr FdManager::get(int fd, bool auto_create)
    {
        if (fd == -1)
        {
            return nullptr;
        }
        // 集合中没有，并且不自动创建，返回nullptr
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_datas.size() <= fd)
        {
            if (auto_create == false)
            {
                return nullptr;
            }
        }
        else
        {
            if (m_datas[fd] || !auto_create)
            {
                return m_datas[fd];
            }
        }
        lock.unlock();

        RWMutexType::WriteLock lock2(m_mutex);
        // 创建新的FdCtx
        FdCtx::ptr ctx(new FdCtx(fd));
        // fd比集合下标打，扩充
        if (fd >= (int)m_datas.size())
        {
            m_datas.resize(fd * 1.5);
        }
        // 放入集合中
        m_datas[fd] = ctx;
        return ctx;
    }
    // 删除文件句柄类FdCtx
    void FdManager::del(int fd)
    {
        RWMutexType::WriteLock lock(m_mutex);
        if ((int)m_datas.size() <= fd)
        {
            return;
        }
        m_datas[fd].reset();
    }
}