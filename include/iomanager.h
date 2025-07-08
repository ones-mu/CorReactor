
#pragma once
#include "scheduler.h"
#include "timer.h"

namespace version04
{
    class IOManager : public Scheduler, public TimerManager
    {
    public:
        using ptr = std::shared_ptr<IOManager>;
        using RWMutexType = version04::RWMutex;
        // 这个选择的值是和epoll检测的值对应的是一样的 #include <sys/epoll.h>
        enum class Event
        {
            /// 无事件
            NONE = 0x0,
            /// 读事件(EPOLLIN)  对应EPOLLIN
            READ = 0x1,
            /// 写事件(EPOLLOUT)  对应EPOLLOUT
            WRITE = 0x4,
            /// 错误事件(EPOLLERR) EPOLLERR
            // ERROR = 0x8,
        };

    private:
        /// Socket事件上线文类
        // 一个事件的类来装这些事件  fd是句柄
        struct FdContext
        {
            using MutexType = version04::Mutex;
            /// 事件上线文类
            // 每种事件有想要的实例
            struct EventContext
            {
                /// 事件执行的调度器
                Scheduler *scheduler = nullptr; // 要在哪一个scheduler上执行  事件执行的scheduler
                /// 事件协程
                Fiber::ptr fiber; // 事件的协程
                /// 事件回调函数
                std::function<void()> cb; // 事件的回调函数
            };

            /**
             * @brief 获取事件上下文
             * @param[in] event 事件类型
             * @return 返回对应事件的上下文
             */
            EventContext &getContext(Event event);
            /**
             * @brief 重置事件上下文
             * @param[in] ctx 待重置的事件上下文
             */
            void resetContext(EventContext &ctx);
            /**
             * @brief 触发事件
             * @param[in] event 事件类型
             */
            void triggerEvent(Event event);

            int fd;                       // 事件关联的句柄
            EventContext read;            // 读事件事件上写文
            EventContext write;           // 写事件上下文
            Event m_events = Event::NONE; // 当前的事件
            MutexType mutex;              // 互斥锁
        };

    public:
        IOManager(size_t thread_num = 1, bool use_caller = true, const std::string &name = "");
        ~IOManager();
        /**
         * @brief 添加事件
         * @param[in] fd socket句柄
         * @param[in] event 事件类型
         * @param[in] cb 事件回调函数
         * @return 添加成功返回0,失败返回-1
         */
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        /**
         * @brief 删除事件
         * @param[in] fd socket句柄
         * @param[in] event 事件类型
         * @attention 不会触发事件
         */
        bool delEvent(int fd, Event event);
        /**
         * @brief 取消事件
         * @param[in] fd socket句柄
         * @param[in] event 事件类型
         * @attention 如果事件存在则触发事件
         */
        // 取消事件就是把fd的读/写事件需要执行的功能触发，然后取消fd的读或写事件
        bool cancelEvent(int fd, Event event);
        bool cancelAll(int fd); // 取消fd的所有事件
        /**
         * @brief 返回当前的IOManager
         */
        static IOManager *GetThis();

    protected:
        void tickle() override;
        bool stopping() override;
        void idle() override;
        void onTimerInsertedAtFront() override;

        /**
         * @brief 重置socket句柄上下文的容器大小
         * @param[in] size 容量大小
         */
        void contextResize(size_t size);

        /**
         * @brief 判断是否可以停止
         * @param[out] timeout 最近要出发的定时器事件间隔
         * @return 返回是否可以停止
         */
        bool stopping(uint64_t &timeout);

    private:
        int m_epfd = 0;
        int m_tickleFds[2];                         // 用于pip2 或evevtfd 认为的唤醒epoll_wait的阻塞   也可以用socketpair吧
        std::atomic<size_t> m_pendingEventCount{0}; // 待处理的事件数量
        RWMutexType m_mutex;                        // 读写锁
        /// socket事件上下文的容器
        std::vector<FdContext *> m_fdContexts;
    };
}