#pragma once
#include <cstdint>
#include <set>
#include <functional>
#include <memory>

#include "thread.h"
namespace version04
{
    class TimerManager;
    /**
     * @brief 定时器
     *
     */
    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;

    public:
        using ptr = std::shared_ptr<Timer>;
        // 取消定时器
        bool cancel();
        // 刷新设置定时器的执行时间
        bool refresh();
        /**
         * @brief 重置定时器时间
         * @param[in] ms 定时器执行间隔时间(ms)
         * @param[in] from_now 是否从当前时间开始计算
         *
         */
        bool reset(uint64_t ms, bool from_now);

    private:
        /**
         * @brief 构造函数
         * @param[in] ms 定时器执行间隔时间
         * @param[in] cb 回调函数
         * @param[in] recurring 是否循环
         * @param[in] manager 定时器管理器
         */
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);
        /**
         * @brief 构造函数
         * @param[in] next 执行的时间戳(毫秒)
         */
        Timer(uint64_t next);

    private:
        /// 是否循环定时器
        bool m_recurring = false;
        /// 执行周期
        uint64_t m_ms = 0;
        /// 精确的执行时间
        uint64_t m_next = 0;
        /// 回调函数
        std::function<void()> m_cb;
        /// 定时器管理器
        TimerManager *m_manager = nullptr;

    private:
        /**
         * @brief 定时器比较仿函数
         */
        struct Comparator
        {
            /// 比较定时器的智能指针的大小（按执行时间排序）
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
        };
    };

    /**
     * @brief 定时器管理器
     */
    class TimerManager
    {
        friend class Timer;

    public:
        using RWMutexType = version04::RWMutex;
        /**
         * @brief 构造函数
         */
        TimerManager();

        /**
         * @brief 析构函数
         */
        virtual ~TimerManager();

        /**
         * @brief 添加定时器
         * @param[in] ms 定时器执行间隔时间
         * @param[in] cb 定时器回调函数
         * @param[in] recurring 是否循环定时器
         */
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
        /**
         * @brief 添加条件定时器
         * @param[in] ms 定时器执行间隔时间
         * @param[in] cb 定时器回调函数
         * @param[in] weak_cond 条件
         * @param[in] recurring 是否循环
         *
         * 当时间到了的时候 条件要是不满足了 就不会执行了
         */
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                     std::weak_ptr<void> weak_cond, bool recurring = false);

        /**
         * @brief 到最近一个定时器执行的时间间隔（ms）
         */
        uint64_t getNextTimer();
        /**
         * @brief 获取需要执行的定时器的回调函数列表
         * @param[in] cbs 回调函数数组
         * 到时候应该epoll_wait 将这些添加到协程中吧
         */
        void listExpiredCb(std::vector<std::function<void()>> &cbs);
        /**
         * @brief 是否有定时器  判空操作
         */
        bool hasTimer();
    protected:
        /**
         * @brief 当有新的定时器插入到定时器的首部，执行该函数 
         * 在IOmanager重写这个函数的时候，用的是执行tickle函数  ：通知协程调度器有任务了      *
         */
        virtual void onTimerInsertedAtFront() = 0;
        /**
         * @brief 将定时器添加到管理器中  重载函数
         */
        void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

    private:
        /**
         * @brief 检测服务器时间是否被调后了
         */
        bool detectClockRollover(uint64_t now_ms);

    private:
        /// Mutex
        RWMutexType m_mutex;
        /// 定时器集合
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        /// 是否触发onTimerInsertedAtFront
        bool m_tickled = false;
        /// 上次执行时间
        uint64_t m_previouseTime = 0;
    };
}