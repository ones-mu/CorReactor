#include "scheduler.h"

namespace version04
{
    //当前线程的协程调度器，同一个调度器下的所有线程指向同一个调度器实例
    static thread_local Scheduler* t_scheduler=nullptr;
    //当前线程的调度协程，每个线程都有一份，包括caller线程
    static thread_local Fiber* t_scheduler_fiber=nullptr;
    //t_scheduler_fiber保存当前线程的调度协程，加上Fiber模块的t_fiber和t_thread_fiber，每个线程总共可以记录三个协程的上下文信息


    Scheduler::Scheduler(size_t threads,bool use_caller,const std::string &name):m_name(name),m_useCaller(use_caller)
    {
        VERSION04_ASSERT2(threads>0,"Scheduler::Scheduler number of inputs threads should be more than one ");
        // m_useCaller=use_caller;
        // m_name=name;
        if(m_useCaller)
        {
            version04::Fiber::GetThis(); //这个是获得当前正在运行的协程，如果没有正在运行的会初始化一个主协程
            --threads; //use_caller也算一个线程
            //在创建携程调度器的时候 应该保证一个线程只有一个携程调度器（这种是不是一般普遍用单例模式实现？）
            VERSION04_ASSERT2(GetThis()==nullptr,"The thread already has a coroutine scheduler");
            t_scheduler=this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this)));//建立了一个子协程做调度器的调度协程？
            t_scheduler_fiber=m_rootFiber.get();
            m_rootThread=version04::GetThreadId();
            m_threadIds.push_back(m_rootThread);//放到线程Id的列表中
        }else
        {
            m_rootThread=-1;
        }
        m_threadCount=threads;
    }

    Scheduler::~Scheduler()
    {
        VERSION04_ASSERT(m_stopping);
        if(GetThis()==this)
        {
            t_scheduler=nullptr;
        }
    }

    Scheduler* Scheduler::GetThis()
    {
        return t_scheduler;
    }
    void Scheduler::SetThis()
    {
        t_scheduler=this;
    }

    //返回的是调度协程
    Fiber* Scheduler::GetMainFiber()
    {
        return t_scheduler_fiber;
    }

    //一个
    void Scheduler::start()
    {
        Mutex::Lock lock(m_mutex);
        if(!m_stopping) return;
        m_stopping=false;
        VERSION04_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);
        //初始化了需要数目的子线程，子线程会立即开始执行对应的run，run就要考虑所处在的线程的问题，对于子线程要去初始化协程
        for(size_t i=0;i<m_threadCount;++i)
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this),m_name+"_"+std::to_string(i)));
            m_threadIds.emplace_back(m_threads[i]->getId());
        }
        lock.unlock();
        
        //现在这个是主线程，主线程接下来该做什么呢？依靠协程调度器也进行协程的调度？
        //主线程调度的逻辑竟然在stop中
        // if(m_rootFiber)
        // {
        //     // m_rootFiber->call();
        // }
    }
    void Scheduler::run()
    {
        ULOG_INFO("main","Scheduler::run");
        //首先看是不是子线程，还要看这个子线程是不是初次调用
        if(m_rootThread!=version04::GetThreadId())
        {
            //如果该线程是初次调用，（因为我们假设协程调度器只有一个，那这个run除了主线程以外就是子线程第一次调用）
            //线程中数据非栈上数据都是共同的，所以说，我们不去修改主线程id，而是只是操作该线程相关的东西
            t_scheduler_fiber=version04::Fiber::GetThis().get();//这样设置是不是说对于子线程来说其 调度协程和协程对象上设置的主协程设置的是一个？
        }
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle,this)));//当任务队列空闲的时候执行这个协程
        Fiber::ptr cb_fiber;
        //每个线程去协程调度器的队列里面去那合适的协程进行运行

        ScheduleTask ft;
        //每个线程应该是循环执行这部分的
        while(true)
        {
            ft.reset();//这个reset是这个类中自定义的函数， 目的是全部置9
            bool tickle_me=false;
            bool is_active=false;

            MutexType::Lock lock(m_mutex);
            auto it=m_fibers.begin();
            while(it!=m_fibers.end())
            {
                //然后就是要找符合条件的了 第一个排除的就是有指定了特定的线程运行的了
                if(it->thread!=-1&&it->thread!=version04::GetThreadId())
                {
                    ++it;
                    tickle_me=true;
                    continue;
                }
                VERSION04_ASSERT(it->fiber||it->cb);//封装的类，或者说传进来的function或者说协程至少要有一个是存在的，否则就是在执行nullptr
                //这个如果正在执行了，就跳过去
                if(it->fiber&&it->fiber->getState()==version04::Fiber::State::EXEC)
                {
                    ++it;
                    continue;
                }
                //找到了 要拿来处理了
                ft=*it;
                m_fibers.erase(it);
                ++m_activateThreadCount;
                is_active=true;
                break;
            }
            //说明遇到了有指定特定线程的情况
            if(tickle_me)
            {
                tickle();
            }   
            
            //对于不同的协程状态做出不一样的操作
            if(ft.fiber&&(ft.fiber->getState()!=Fiber::State::TERM&&ft.fiber->getState()!=Fiber::State::EXCEPT))
            {
                ft.fiber->swapIn();//根据我们的设置，运行的协程执行完毕后没有跳转，m_ctx.uc_link 设置为了nullptr
                --m_activateThreadCount;

                //按道理、至少从现阶段的代码来看，swapIn()后 修改的状态是EXEC 不知还有哪里会修改这个运行状态
                if(ft.fiber->getState()==Fiber::State::READY)
                {
                    schedule(ft.fiber);//放入队列中
                }else if(ft.fiber->getState()!=Fiber::State::TERM&&ft.fiber->getState()!=Fiber::State::EXCEPT)
                {
                    ft.fiber->m_state=Fiber::State::HOLD;
                }
                ft.reset();
            }else if(ft.cb)
            {
                if(cb_fiber)
                {
                    cb_fiber->reset(ft.cb);//这个fiber类中对应的函数，目的是重复利用内存，
                }else
                {
                    ULOG_INFO("main","callback address: {}",reinterpret_cast<void*>(ft.cb));
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                ++m_activateThreadCount;
                cb_fiber->swapIn();
                --m_activateThreadCount;
                if(cb_fiber->getState()==Fiber::State::READY)
                {
                    schedule(cb_fiber);//将构建好的协程放里面，
                    cb_fiber.reset();
                }else if(cb_fiber->getState()==Fiber::State::EXCEPT||cb_fiber->getState()==Fiber::State::TERM)
                {
                    cb_fiber->reset(nullptr);
                }else
                {
                    cb_fiber->m_state=Fiber::State::HOLD;
                    cb_fiber.reset();
                }
            }else
            {
                if(is_active)
                {
                    --m_activateThreadCount;
                    continue;
                }
                if(idle_fiber->getState()==Fiber::State::TERM)
                {
                    ULOG_INFO("main","idle fiber term");
                    break;//这个是得到了stop退出命令 跳出循环，然后使线程离开作用域，类对象析构了 这个对应的线程的运行也就结束了
                }

                idle_fiber->swapIn();
                --m_idleThreadCount;
                if(idle_fiber->getState()!=Fiber::State::TERM&&idle_fiber->getState()!=Fiber::State::EXCEPT)
                {
                    idle_fiber->m_state=Fiber::State::HOLD;
                }
            }
        }
    }



    void Scheduler::tickle()
    {
        ULOG_INFO("main","tickle");
    }
    void Scheduler::idle()
    {
        ULOG_INFO("main","idle");
        while (!stopping())
        {
            version04::Fiber::YieldToHold();
        }
        
    }
    //我看这个stop也只有主线程会调用吧？
    //主线程和子线程分离的逻辑竟然在这里，那也就是没有线程手动回收了，完全以类对象的形式结束作用域进行回收
    //这里这个逻辑设置有问题吧？ 这个开始的时候判断？那
    void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::State::TERM
                || m_rootFiber->getState() == Fiber::State::INIT)) {
        ULOG_INFO("main","stopped");
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    //bool exit_on_this_fiber = false;
    if(m_rootThread != -1) {
        VERSION04_ASSERT(GetThis() == this);
    } else {
        VERSION04_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }

    if(m_rootFiber) {
        //while(!stopping()) {
        //    if(m_rootFiber->getState() == Fiber::TERM
        //            || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    m_rootFiber->call();
        //}
        if(!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
    //if(exit_on_this_fiber) {
    //}
}

    //这个是判断是否要停止 还是是否已经停下来了
    bool Scheduler::stopping()
    {
        //上了该协程调度器的锁
        MutexType::Lock lock(m_mutex);
        return m_autoStop&&m_stopping&&m_fibers.empty()&&m_activateThreadCount==0;
    }


}