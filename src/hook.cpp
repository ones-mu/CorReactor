#include "hook.h"
#include "macro.h"
#include "thread.h"
#include "controllogger.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "fiber.h"
#include "macro.h"
#include <dlfcn.h>
#include <memory>
#include <vector>
#include <sys/socket.h>
#include <errno.h>
#include <cstdarg> // 提供可变参数支持的头文件
#include <fcntl.h>
#include <sys/ioctl.h>

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

namespace version04
{
    static thread_local bool t_hook_enable = false;

    void hook_init()
    {
        static bool is_inited = false;
        if (is_inited)
        {
            return;
        }
/*
 // dlsym - 从一个动态链接库或者可执行文件中获取到符号地址。成功返回跟name关联的地址
// RTLD_NEXT 返回第一个匹配到的 "name" 的函数地址
// 取出原函数，赋值给新函数
*/
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }

    /*
        宏展开如下:
        extern "C" {
        sleep_fun sleep_f = nullptr;
        usleep_fun usleep_f = nullptr;
        .....
        setsocketopt_fun setsocket_f = nullptr;
    }

    void hook_init() {
        static bool is_inited = false;
        if (is_inited) {
            return;
        }

        sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
        usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep");
        ...
        setsocketopt_f = (setsocketopt_fun)dlsym(RTLD_NEXT, "setsocketopt");
    }
    */

    static uint64_t s_connect_timeout = -1;
    struct _HookIniter
    {
        _HookIniter()
        {
            hook_init();
            s_connect_timeout = 5000; // 我这里暂时没实现带热更新的动态加载，先写死
        }
    };
    // 创建其静态对象，这样在main函数执行前就完成了初始化，运行了_HookIniter的构造函数包含的函数
    static _HookIniter s_hook_initer;
    bool is_hook_enable()
    {
        return t_hook_enable;
    }
    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;
    }
}
// 定时器超时条件
struct timer_info
{
    int cancelled = 0;
};
/*
 * 	fd 			 	文件描述符
 * 	fun				原始函数
 *	hook_fun_name	hook的函数名称
 *	event			事件
 *	timeout_so		超时时间类型
 *	args			可变参数
 *
 * 	例如：return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
 */
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name, uint32_t event, int timeout_so, Args &&...args)
{
    if (!version04::t_hook_enable)
    {
        return fun(fd, std::forward<Args>(args)...);
    }
    version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(fd);
    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }
    // 查看文件描述符是否关闭
    if (ctx->isClose())
    {
        // 坏文件描述符
        errno = EBADF;
        return -1;
    }
    // 不是socket 或 用户设置了非阻塞
    if (!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }
    // 用hook实现同步io的使用方法但实际上异步io
    // 获得超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    // 设置超时条件
    std::shared_ptr<timer_info> tinfo(new timer_info);
retry:
    // 先执行fun 读数据或写数据 若函数返回值有效就直接返回
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    ULOG_DEBUG_SRC("system", "do_in {} > n= {}", hook_fun_name, n);
    // 若中断则重试
    while (n == -1 && errno == EINTR)
    {
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 若为阻塞状态
    if (n == -1 && errno == EAGAIN)
    {
        // 重置EAGIN(errno=11)，此处已处理，不在向上返回该错误
        // 这个应该是非阻塞状态下的资源暂时不可用，请重试吧
        errno = 0;
        // 获得当前io调度器
        version04::IOManager *iom = version04::IOManager::GetThis();
        // 定时器
        version04::Timer::ptr timer;
        // tinfo的弱指针 可以判断tinfo是否已经销毁
        std::weak_ptr<timer_info> winfo(tinfo);

        // 说明设置了超时时间
        if (to != (uint64_t)-1)
        {
            /*添加条件定时器
            to时间消息还没来就触发callback
            */
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]()
                                           {
                auto t=winfo.lock();
                // tinfo失效//设了错误 定时器失效了
                if(!t||t->cancelled)
                {
                    return;
                }
                //没错误的话设置为超时而失效
                t->cancelled=ETIMEDOUT;
                //取消事件强制唤醒
                iom->cancelEvent(fd,(version04::IOManager::Event)(event)); }, winfo);
        }
        // addEvent error:-1 acc:0 cb为空，任务为执行当前协程
        int rt = iom->addEvent(fd, (version04::IOManager::Event)(event));
        // addEvent 失效，取消上面加的定时器
        if (SYLAR_UNLIKELY(rt == -1))
        {
            ULOG_ERROR_SRC("system", "hook_fun_name addEvent(fd:{},event:{})", fd, event);
            if (timer)
            {
                timer->cancel();
            }
            return -1;
        }
        else
        {
            /*
            addEvent成功，把执行事件让出来(就是添加的两个事件(依据当前协程))
            只有两种情况会从这回来
            * 	1) 超时了， timer cancelEvent triggerEvent会唤醒回来
            * 	2) addEvent数据回来了会唤醒回来
            */
            version04::Fiber::YieldToHold();
            if (timer)
            {
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }
    return n;
}

extern "C"
{
#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX);
#undef XX
    unsigned int sleep(unsigned int seconds)
    {
        if (!version04::t_hook_enable)
        {
            return sleep_f(seconds);
        }
        // 获取当前的协程
        version04::Fiber::ptr fiber = version04::Fiber::GetThis();
        version04::IOManager *iom = version04::IOManager::GetThis();
        iom->addTimer(seconds * 1000, std::bind((void (version04::Scheduler::*)(version04::Fiber::ptr, int thread))&version04::IOManager::schedule,
                                                iom, fiber, -1));

        /*
            //这个等价于
            iom->addTimer(seconds*1000, [iom,fiber](){
                iom->schedule(fiber,-1);})
        */

        version04::Fiber::YieldToHold(); // 切回到调度协程
        return 0;
    }
    int usleep(useconds_t usec)
    {
        if (!version04::t_hook_enable)
        {
            return usleep_f(usec);
        }
        version04::Fiber::ptr fiber = version04::Fiber::GetThis();
        version04::IOManager *iom = version04::IOManager::GetThis();
        iom->addTimer(usec / 1000, std::bind((void (version04::Scheduler::*)(version04::Fiber::ptr, int thread))&version04::IOManager::schedule, iom, fiber, -1));
        version04::Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem)
    {
        if (!version04::t_hook_enable)
        {
            return nanosleep_f(req, rem);
        }

        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        version04::Fiber::ptr fiber = version04::Fiber::GetThis();
        version04::IOManager *iom = version04::IOManager::GetThis();
        iom->addTimer(timeout_ms, std::bind((void (version04::Scheduler::*)(version04::Fiber::ptr, int thread))&version04::IOManager::schedule, iom, fiber, -1));
        version04::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol)
    {
        if (!version04::t_hook_enable)
        {
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        if (fd == -1)
        {
            return fd;
        }
        // 参数true表示没有会自动创建
        version04::FdMgr::GetInstance()->get(fd, true); // 这么做是为了记录更多的信息
        // 看样子句柄还是句柄，只是放到了文件句柄合集vector里
        return fd;
    }
    // socket通信客户端的connect函数
    int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        if (!version04::t_hook_enable)
        {
            return connect_f(fd, addr, addrlen);
        }
        // 从文件描述符管理队列中去寻找，管理的有没有这个fd
        version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(fd);
        // 如果没有管理，返回-1,也就是只要是t_hook_enable就必须有管理？
        if (!ctx || ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }
        // 是否是socket，如果不是，直接调用原函数？？？ 不是socket 怎么能connect？
        if (!ctx->isSocket())
        {
            return connect_f(fd, addr, addrlen);
        }
        if (ctx->getUserNonblock())
        {
            return connect_f(fd, addr, addrlen);
        }
        // 要求都满足了，开始异步逻辑
        int n = connect_f(fd, addr, addrlen);
        // 连接成功
        if (n == 0)
        {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS)
        {
            // 其他错误，EINPROGRESS表示连接操作正在进行中
            // 即出现了错误，但错误不是正在连接中那返回，表示出现了错误
            return n;
        }

        version04::IOManager *iom = version04::IOManager::GetThis();
        version04::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        // timeout_ms不是最大最大值就意味着设置了超时时间
        // 这里是-1进行了类型转换
        if (timeout_ms != (uint64_t)-1)
        {
            // 加条件定时器
            timer = iom->addConditionTimer(timeout_ms, [iom, fd, winfo]()
                                           {
                    auto t=winfo.lock();
                    if(!t||t->cancelled)
                    {
                        return;
                    }
                    t->cancelled=ETIMEDOUT;
                    iom->cancelEvent(fd,version04::IOManager::Event::WRITE); }, winfo);
        }
        // 注册写事件 cb为空，说明将当前协程注册到里面
        int rt = iom->addEvent(fd, version04::IOManager::Event::WRITE);
        if (rt == 0)
        {
            // 只有两种情况唤醒，1.超时从定时器唤醒 2.连接成功 从epoll_wait中拿到事件
            version04::Fiber::YieldToHold();
            // 当前协程切换到了调度协程了，按我的理解后面应该不会执行了啊？
            // 因为前面cb为空，所以将当前协程注册到了里面
            if (timer)
                timer->cancel();
            // 从定时器唤醒，超时失败
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else
        {
            // 添加事件失败
            if (timer)
            {
                timer->cancel();
            }
            ULOG_ERROR_SRC("system", "connect addEvent({},WRITE) error", fd);
        }
        int error = 0;
        socklen_t len = sizeof(error);
        // 获取套接字的错误状态
        if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }
        // 没有错误，连接成功
        if (!error)
        {
            return 0;
        }
        else
        {
            // 有错误，连接失败
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        return connect_with_timeout(sockfd, addr, addrlen, version04::s_connect_timeout);
    }
    int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = do_io(s, accept_f, "accept", static_cast<uint32_t>(version04::IOManager::Event::READ), SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0)
        {
            version04::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    ssize_t read(int fd, void *buf, size_t count)
    {
        return do_io(fd, read_f, "read", static_cast<uint32_t>(version04::IOManager::Event::READ), SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", static_cast<uint32_t>(version04::IOManager::Event::READ), SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return do_io(sockfd, recv_f, "recv", static_cast<uint32_t>(version04::IOManager::Event::READ), SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", static_cast<uint32_t>(version04::IOManager::Event::READ), SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return do_io(sockfd, recvmsg_f, "recvmsg", static_cast<uint32_t>(version04::IOManager::Event::READ), SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count)
    {
        return do_io(fd, write_f, "write", static_cast<uint32_t>(version04::IOManager::Event::WRITE), SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", static_cast<uint32_t>(version04::IOManager::Event::WRITE), SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int s, const void *msg, size_t len, int flags)
    {
        return do_io(s, send_f, "send", static_cast<uint32_t>(version04::IOManager::Event::WRITE), SO_SNDTIMEO, msg, len, flags);
    }

    ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
    {
        return do_io(s, sendto_f, "sendto", static_cast<uint32_t>(version04::IOManager::Event::WRITE), SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
    {
        return do_io(s, sendmsg_f, "sendmsg", static_cast<uint32_t>(version04::IOManager::Event::WRITE), SO_SNDTIMEO, msg, flags);
    }

    int close(int fd)
    {
        if (!version04::t_hook_enable)
        {
            return close_f(fd);
        }

        version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(fd);
        if (ctx)
        {
            auto iom = version04::IOManager::GetThis();
            if (iom)
            {
                iom->cancelAll(fd);
            }
            version04::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    // 这里面用了c语言中的可变参数
    int fcntl(int fd, int cmd, ... /* arg */)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if (ctx->getSysNonblock())
            {
                arg |= O_NONBLOCK;
            }
            else
            {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFL:
        {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return arg;
            }
            if (ctx->getUserNonblock())
            {
                return arg | O_NONBLOCK;
            }
            else
            {
                return arg & ~O_NONBLOCK;
            }
        }
        break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
        break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
        }
    }

    int ioctl(int d, unsigned long int request, ...)
    {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void *);
        va_end(va);

        if (FIONBIO == request)
        {
            bool user_nonblock = !!*(int *)arg;
            version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!version04::t_hook_enable)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                version04::FdCtx::ptr ctx = version04::FdMgr::GetInstance()->get(sockfd);
                if (ctx)
                {
                    const timeval *v = (const timeval *)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}
