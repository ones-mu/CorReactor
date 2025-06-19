#include "thread.h"
#include "util.h"
#include <stdexcept>

namespace version04
{
    static thread_local Thread* t_thread=nullptr;
    static thread_local std::string t_thread_name="UNKNOWN";

    
    Thread *Thread::GetThis()
    {
        return t_thread;
    }
    
    const std::string &Thread::GetName()
    {
        return t_thread_name;
    }
    
    void Thread::SetName(const std::string &name)
    {
        if(t_thread)
        {
            t_thread->m_name=name;
        }
        t_thread_name=name;
    }
    Thread::Thread(std::function<void()> cb, const std::string &name):m_cb(cb),m_name(name)
    {
        if(name.empty())
        {
            m_name="UNKNOWN";
        }
        int rt=pthread_create(&m_thread,nullptr,Thread::run,this); 
        if(rt!=0)
        {

            throw std::logic_error("pthread_create error");
        }
    }
    Thread::~Thread()
    {
        if(m_thread)
        {
            pthread_detach(m_thread);
        }
    }
    void Thread::join()
    {
        if(m_thread)
        {
            int rt=pthread_join(m_thread,nullptr);//这个类的创建和类相关函数的调用是主线程做的，只有Thread::run其实是子线程执行的
            if(rt)
            {
                throw std::logic_error("pthread_join error");
            }
        }
    }

    void* Thread::run(void* arg)
    {
        //这个函数的运行都是在子线程下运行的
        Thread* thread =(Thread*)arg;
        //t_thread与t_thread_name的值都是具有线程独立性的
        t_thread=thread;
        t_thread_name=thread->m_name;
        thread->m_id=version04::GetThreadId();
        //给线程id重命名，只能是16个字符
        //pthread_setname_np(pthread_self(),thread->m_name.c_str()); 是linux中特有的函数
        pthread_setname_np(pthread_self(),thread->m_name.substr(0,16).c_str()); 

        std::function<void()> cb;
        cb.swap(thread->m_cb); // 任务转移到 cb，thread->m_cb 变为空

        cb();
        return 0;
    }
}