/*
轻量级的线程、协程放在线程上跑、线程是协程的容器
协程在做高并发的一些任务

c :: pthread
c++ std::thread

*/

#pragma once
#include <pthread.h>
#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <stdint.h>    //uint32_t
#include <semaphore.h> //sem_t

namespace version04
{

    class Semphore
    {
    public:
        Semphore(uint32_t count = 0);
        ~Semphore();
        void wait();
        void notify();

    private:
        Semphore(const Semphore &) = delete;
        Semphore(const Semphore &&) = delete;
        Semphore &operator=(const Semphore &) = delete;

        sem_t m_semaphore;
    };
    template <class T>
    struct ScopedLockImpl
    {
    public:
        ScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    template <class T>
    struct ReadScopedLockImpl
    {
    public:
        ReadScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    template <class T>
    struct WriteScopedLockImpl
    {
    public:
        WriteScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    class RWMutex
    {
    public:
        typedef ReadScopedLockImpl<RWMutex> ReadLock;
        typedef WriteScopedLockImpl<RWMutex> WriteLock;

        RWMutex()
        {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        ~RWMutex()
        {
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock()
        {
            pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock()
        {
            pthread_rwlock_wrlock(&m_lock);
        }

        void unlock()
        {
            pthread_rwlock_unlock(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock;
    };

    class Thread
    {
    public:
        // typedef std::shared_ptr<Thread> ptr;
        using ptr = std::shared_ptr<Thread>;
        Thread(std::function<void()> cb, const std::string &name = "");
        ~Thread();

        pid_t getId() const { return m_id; }
        const std::string &getName() const { return m_name; }
        void join();
        static Thread *GetThis();            // 获取当前线程对象指针
        static const std::string &GetName(); // 获取当前线程名称
        static void SetName(const std::string &name);

    private:
        // 禁止拷贝构造
        Thread(const Thread &) = delete;
        Thread(const Thread &&) = delete;
        Thread &operator=(const Thread &) = delete;

        static void *run(void *arg);

        pid_t m_id = -1; // 线程id
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;

        Semphore m_semphore; // 信号量
    };
}
