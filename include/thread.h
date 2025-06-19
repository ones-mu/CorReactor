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


namespace version04
{
    class Thread
    {
    public:
        // typedef std::shared_ptr<Thread> ptr;
        using ptr = std::shared_ptr<Thread>;
        Thread(std::function<void()> cb, const std::string& name = "");
        ~Thread();

        pid_t getId() const {return m_id;} 
        const std::string& getName() const {return m_name;}
        void join();
        static Thread* GetThis(); //获取当前线程对象指针
        static const std::string& GetName(); //获取当前线程名称
        static void SetName(const std::string& name);
    private:
    //禁止拷贝构造
        Thread(const Thread&)=delete;
        Thread(const Thread&&)=delete;
        Thread& operator=(const Thread&)=delete;
        
        static void* run(void* arg);

        pid_t m_id=-1; //线程id
        pthread_t m_thread=0;
        std::function<void()> m_cb; 
        std::string m_name;
        
    };
}
