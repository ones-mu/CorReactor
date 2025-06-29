#include "iomanager.h"
#include "controllogger.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <macro.h>

namespace version04
{
    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event)
    {
        switch (event)
        {
        case Event::READ:
            return read;
        case Event::WRITE:
            return write;
        default:
            VERSION04_ASSERT2(false, "IOManager::FdContext::getContext invalid event");
        }
    }
    void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset(); // 智能指针的reset方法
        ctx.cb = nullptr;
    }
    void IOManager::FdContext::triggerEvent(IOManager::Event event)
    {
        VERSION04_ASSERT2(static_cast<int>(event) & static_cast<int>(m_events), "IOManager::FdContext::triggerEvent if is the same event");
        m_events = (Event)(static_cast<int>(m_events) & ~static_cast<int>(event));
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            ctx.scheduler->schedule(ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(ctx.fiber);
        }
        return;
    }
    /*
        这个构造函数主要是进行了epoll的初始化，并且将唤醒操作的文件描述符使用epoll_ctl进行注册
    */
    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) : Scheduler(threads, use_caller, name)
    {
        m_epfd = epoll_create(1);
        VERSION04_ASSERT2(m_epfd > 0, "IOManager::IOManager epoll_create failed");

        // 成功的时候返回0，失败的时候返回-1
        int rt = pipe(m_tickleFds);
        VERSION04_ASSERT2(!rt, "IOManager::IOManager pipe failed");

        epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN | EPOLLET; // 检测读事件，并且设置为边缘触发
        event.data.fd = m_tickleFds[0];
        // 修改文件描述符的属性，使其是非阻塞的
        //  rt=fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        int flag = fcntl(m_tickleFds[0], F_GETFL);
        flag |= O_NONBLOCK; // 添加非阻塞属性
        fcntl(m_tickleFds[0], F_SETFL, flag);
        // 注册唤醒操作的文件描述符
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        VERSION04_ASSERT2(!rt, "IOManager::IOManager epoll_ctl failed");

        contextResize(32); // 初始化事件上下文数组的尺寸
        start();           // 父类函数
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext();
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        // RWMutexType::ReadLock lock(m_mutex);
        // //感觉这个m_fdContexts用unordered_map会更好
        // if((int)m_fdContexts.size()>fd)
        // {
        //     fd_ctx=m_fdContexts[fd];
        //     lock.unlock();
        // }else
        // {
        //     lock.unlock();
        //     RWMutexType::WriteLock write_lock(m_mutex);
        //     contextResize(fd*2);
        //     fd_ctx=m_fdContexts[fd];
        // }
        // //应该改为while
        // while(1)
        // {
        //     RWMutexType::ReadLock lock(m_mutex);
        //     if((int)m_fdContexts.size()>fd)
        //     {
        //         fd_ctx=m_fdContexts[fd];
        //         lock.unlock();
        //         break;
        //     }else
        //     {
        //         lock.unlock();
        //         RWMutexType::WriteLock write_lock(m_mutex);
        //         contextResize(fd*2);
        //     }
        // }
        while (true)
        {
            // 先尝试获取读锁
            RWMutexType::ReadLock read_lock(m_mutex);
            if ((int)m_fdContexts.size() > fd)
            {
                fd_ctx = m_fdContexts[fd];
                break; // 找到后直接退出循环 可以直接break是因为break后就离开作用域了，锁会释放
            }
            read_lock.unlock(); // 释放读锁

            // 需要扩容，获取写锁
            RWMutexType::WriteLock write_lock(m_mutex);
            // 再次检查，防止其他线程已经扩容
            if ((int)m_fdContexts.size() > fd)
            {
                fd_ctx = m_fdContexts[fd];
                break; // 其他线程已经扩容，直接使用
            }
            // 真正需要扩容
            contextResize(fd * 2);
            // 不需要break，继续循环以获取读锁并检查
        }
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (static_cast<int>(fd_ctx->m_events) & static_cast<int>(event))
        {
            // 这说明已经注册过了
            ULOG_ERROR_SRC("system", "addEvent assert fd={} event={} fd_ctx.event={}", fd, static_cast<uint32_t>(event), static_cast<uint32_t>(fd_ctx->m_events));
            VERSION04_ASSERT(!(static_cast<int>(fd_ctx->m_events) & static_cast<int>(event)));
        }

        int op = static_cast<int>(fd_ctx->m_events) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | static_cast<uint32_t>(fd_ctx->m_events) | static_cast<uint32_t>(event);
        epevent.data.ptr = fd_ctx;

        // 注册事件
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt != 0)
        {
            ULOG_ERROR_SRC("system", "addEvent epoll_ctl failed epoll_ctl={} event={} rt={}", m_epfd, static_cast<uint32_t>(epevent.events), rt);
            return -1;
        }

        ++m_pendingEventCount; // 待处理的事件数量加1
        fd_ctx->m_events = (Event)(static_cast<uint32_t>(fd_ctx->m_events) | static_cast<uint32_t>(event));
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        VERSION04_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            event_ctx.cb.swap(cb);
        }
        else
        {
            event_ctx.fiber = Fiber::GetThis();
            if(event_ctx.fiber->getState() != Fiber::State::EXEC)
            VERSION04_ASSERT2(false, static_cast<int>(event_ctx.fiber->getState()));
        }
        return 0;
    }
    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(static_cast<uint32_t>(fd_ctx->m_events) & static_cast<uint32_t>(event)))
        {
            return false;
        }

        Event new_events = (Event)(static_cast<uint32_t>(fd_ctx->m_events) & ~static_cast<uint32_t>(event));
        int op = static_cast<uint32_t>(new_events) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = static_cast<uint32_t>(EPOLLET) | static_cast<uint32_t>(new_events);
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            ULOG_ERROR_SRC("system", "epoll_ctl(m_epfd:{},op:{},fd:{},epevent.events:{},errno:{},strerror(errno):{})",
                           m_epfd, op, fd, static_cast<uint32_t>(epevent.events), errno, strerror(errno));

            return false;
        }

        --m_pendingEventCount;
        fd_ctx->m_events = new_events;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(static_cast<uint32_t>(fd_ctx->m_events) & static_cast<uint32_t>(event)))
        {
            return false;
        }

        Event new_events = (Event)(static_cast<uint32_t>(fd_ctx->m_events) & ~static_cast<uint32_t>(event));
        int op = static_cast<uint32_t>(new_events) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = (static_cast<uint32_t>(fd_ctx->m_events) | static_cast<uint32_t>(new_events));
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            ULOG_ERROR_SRC("system", "epoll_ctl(m_epfd:{},op:{},fd:{},epevent.events:{},errno:{},strerror(errno):{})",
                           m_epfd, op, fd, static_cast<uint32_t>(epevent.events), errno, strerror(errno));
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(static_cast<uint32_t>(fd_ctx->m_events)))
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            ULOG_ERROR_SRC("system", "epoll_ctl(m_epfd:{},op:{},fd:{},epevent.events:{},errno:{},strerror(errno):{})",
                           m_epfd, op, fd, static_cast<uint32_t>(epevent.events), errno, strerror(errno));
            return false;
        }

        if (static_cast<uint32_t>(fd_ctx->m_events) & static_cast<uint32_t>(Event::READ))
        {
            fd_ctx->triggerEvent(Event::READ); // 触发事件
            --m_pendingEventCount;             // 待处理的事件
        }
        if (static_cast<uint32_t>(fd_ctx->m_events) & static_cast<uint32_t>(Event::WRITE))
        {
            fd_ctx->triggerEvent(Event::WRITE);
            --m_pendingEventCount;
        }

        VERSION04_ASSERT(static_cast<uint32_t>(fd_ctx->m_events) == 0);
        return true;
    }

    IOManager *IOManager::GetThis()
    {
        // 原来的静态方法是返回调度器的指针，现在这个方法是返回io管理器的指针
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    // 唤醒epoll_wait
    void IOManager::tickle()
    {
        if (hasIdleThreads())
        {
            return;
        }
        int rt = write(m_tickleFds[1], "T", 1);
        // 正确rt返回的是1
        VERSION04_ASSERT2(rt, "IOManager::tickle write failed");
    }
    bool IOManager::stopping(uint64_t &timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    bool IOManager::stopping()
    {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOManager::idle()
    {
        ULOG_DEBUG_SRC("main", "IOManager::idle");
        epoll_event *events = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
                                                   { delete[] ptr; });

        while (true)
        {
            uint64_t next_timeout = 0;
            if (stopping(next_timeout))
            {
                ULOG_INFO_SRC("main", "name={} IOManager::idle stopping exit", getName());
                break;
            }

            int rt = 0;
            do
            {
                static const int MAX_TIMEOUT = 3000;
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT
                                       ? MAX_TIMEOUT
                                       : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
                if (rt < 0 && errno == EINTR)
                {
                }
                else
                {
                    break;
                }
            } while (true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            for (int i = 0; i < rt; ++i)
            {
                epoll_event &event = events[i];
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy;
                    while (read(m_tickleFds[0], &dummy, 1) == 1)
                        ;
                    continue;
                }

                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= EPOLLIN | EPOLLOUT;
                }
                int real_events = static_cast<int>(Event::NONE);
                if (event.events & EPOLLIN)
                {
                    real_events |= static_cast<int>(Event::READ);
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= static_cast<int>(Event::WRITE);
                }

                if ((static_cast<uint32_t>(fd_ctx->m_events) & static_cast<uint32_t>(real_events)) == static_cast<uint32_t>(Event::NONE))
                {
                    continue;
                }

                int left_events = (static_cast<int>(fd_ctx->m_events) & ~static_cast<int>(real_events));
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    ULOG_ERROR_SRC("system", "epoll_ctl(m_epfd:{},op:{},fd:{},event.events:{},rt2:{},errno:{},strerror(errno):{})",
                                   m_epfd, op, fd_ctx->fd, static_cast<uint32_t>(event.events), rt2, errno, strerror(errno));
                    continue;
                }

                if (real_events & static_cast<int>(Event::READ))
                {
                    fd_ctx->triggerEvent(Event::READ);
                    --m_pendingEventCount;
                }
                if (real_events & static_cast<int>(Event::WRITE))
                {
                    fd_ctx->triggerEvent(Event::WRITE);
                    --m_pendingEventCount;
                }
            }

            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(size_t i=0;i<m_fdContexts.size();++i)
        {
            if(m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }
}