#include "timer.h"
#include "util.h"
#include "macro.h"
namespace version04
{
    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        if (!lhs && !rhs)
        {
            return false;
        }
        if (!lhs)
        {
            return true;
        }
        if (!rhs)
        {
            return false;
        }
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (rhs->m_next < lhs->m_next)
        {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb,
                 bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
    {
        m_next = version04::GetCurrentMS() + m_ms;
    }
    Timer::Timer(uint64_t next)
        : m_next(next)
    {
    }

    // 取消定时器 从set集合中拿出去
    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        // 如果回调函数还在
        if (m_cb)
        {
            m_cb = nullptr;
            // 从指向的管理者存储的队列中找自己
            auto it = m_manager->m_timers.find(shared_from_this());
            VERSION04_ASSERT(it != m_manager->m_timers.end());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }
    // 刷新设置定时器的执行时间
    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        m_manager->m_timers.erase(it);
        m_next = version04::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }
    // 重置定时器的时间
    bool Timer::reset(uint64_t ms, bool from_now)
    {
        // 和原来一样
        if (ms == m_ms && !from_now)
        {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if (from_now)
        {
            start = version04::GetCurrentMS();
        }
        else
        {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    TimerManager::TimerManager()
    {
        m_previouseTime = version04::GetCurrentMS();
    }
    TimerManager::~TimerManager() {}

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        TimerManager::RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        // 如果weak_cond不是空，weak_cond.lock()会将其转化为shared_ptr
        // 如果weak_cond.lock()返回空，说明已经被回收了，不用执行回调函数
        std::shared_ptr<void> cond = weak_cond.lock();
        // 如果为空，执行了个寂寞
        if (cond)
        {
            cb();
        }
    }
    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }
    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock)
    {
        // 加是不是插入到集合的头部的检测
        auto it = m_timers.insert(val).first;
        // m_tickled 用来判断是否需要唤醒线程 如果未true已经是需要唤醒的了，不用再唤醒了
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front)
        {
            m_tickled = true;
        }
        lock.unlock();
        if (at_front)
        {
            onTimerInsertedAtFront();
        }
    }
    //到最近一个定时器执行的时间间隔
    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if(m_timers.empty())
        {
            return ~0ull;
        }
        const Timer::ptr &next=*m_timers.begin();
        uint64_t now_ms=version04::GetCurrentMS();
        if(now_ms >= next->m_next)
        {
            return 0;
        }else
        {
            return next->m_next - now_ms;
        }
    }


    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = version04::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty())
            return;
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        if (m_timers.empty())
            return;
        bool rollover = detectClockRollover(now_ms);
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        {
            return;
        }
        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());
        for (auto &timer : expired)
        {
            cbs.emplace_back(timer->m_cb);
            if (timer->m_recurring)
            {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb = nullptr;
            }
        }
        /*
        Timer::ptr now_timer;
        for (it = m_timers.begin(); it != m_timers.end();)
        {
            if ((*it)->m_next > now_ms)
            {
                break;
            }
            now_timer = *it;
            it = m_timers.erase(it);
            cbs.emplace_back(now_timer->m_cb);
            if (now_timer->m_recurring)
            {
                now_timer->m_next = now_ms + now_timer->m_ms;
                m_timers.insert(now_timer);
            }
            }
        */
    }

    // 这个函数是用于检测服务器时间是否发生了回拨，如果发生了回拨，则需要重新设置定时器的时间
    // 但设计的有些潦草
    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }
    bool TimerManager::hasTimer()
    {
        // 有上锁操作了 该函数就不能用后置const的形式，表示无修改函数了
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

}