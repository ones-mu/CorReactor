
#pragma once
#include "controllogger.h"
#include "scheduler.h"

namespace version04
{
    class IOManager : public Scheduler
    {
    public:
        using ptr = std::shared_ptr<IOManager>;
        using RWMutexType = version04::RWMutex;

        enum class Event
        {
            ///无事件
            NONE = 0x0,
            ///读事件(EPOLLIN)
            READ = 0x1,
            ///写事件(EPOLLOUT)
            WRITE = 0x4,
            ///错误事件(EPOLLERR)
            // ERROR = 0x8,
        };

    private:
    ///Socket事件上线文类
        // 一个事件的类来装这些事件  fd是句柄
        struct FdContext
        {
            using MutexType = version04::Mutex;
            /// 事件上线文类
            //每种事件有想要的实例
            struct EventContext
            {
                ///事件执行的调度器
                Scheduler* scheduler=nullptr;//要在哪一个scheduler上执行  事件执行的scheduler
                ///事件协程
                Fiber::ptr fiber; //事件的协程
                ///事件回调函数
                std::function<void()> cb; //事件的回调函数
            };
            int fd; //事件关联的句柄
            EventContext in; //读事件
            EventContext out; //写事件
            Event m_events=Event::NONE; //当前的事件
            MutexType mutex; //互斥锁
        };
        public:
            IOManager(size_t thread_num=1,bool use_epoll=true,const std::string& name="");
            ~IOManager();

            int addEvent(int fd,Event event,std::function<void()> cb=nullptr);
            bool delEvent(int fd,Event event);
            //取消事件就是把fd的读/写事件需要执行的功能触发，然后取消fd的读或写事件
            bool cancelEvent(int fd,Event event);
            bool cancelAll(int fd); //取消fd的所有事件

            static IOManager* GetThis();
        protected:
            void tickle() override;
            bool stopping() override;
            void idle() override;

        private:
            int m_epfd=0;
            int m_tickleFds[2];  //用于pip2 或evevtfd 认为的唤醒epoll_wait的阻塞
            std::atomic<size_t> m_pendingEventCount{0}; //待处理的事件数量
            RWMutexType m_mutex; //读写锁
            std::vector<FdContext*> m_fdContexts; 

    };
}