#pragma once
#include "macro.h"
#include "controllogger.h"
#include "util.h"
#include "fiber.h"
#include "thread.h"
#include <vector>
#include <list>
#include <memory>

/*构造函数init start run*/

namespace version04
{
    class Scheduler
    {
    public:
        using ptr = std::shared_ptr<Scheduler>;
        using MutexType = Mutex;
        Scheduler(size_t threads=1,bool use_caller=false,const std::string &name="");
        virtual ~Scheduler();

        const std::string& getName() const {return m_name;}
        static Scheduler* GetThis();
        static Fiber* GetMainFiber();//获取线程的主协程
        void start();
        void stop();
        void run(); //真正在执行协程调度的方法
        //要在协程调度器里面执行协程
        template<class FiberOrcb>
        void schedule(FiberOrcb fc,int thread=-1)
        {
            ULOG_INFO_SRC("main","schedule add");
            bool need_tickle=false;
            {
                MutexType::Lock lock(m_mutex);
                scheduleNoLock(fc,thread);
            }
            if(need_tickle)
            {
                tickle();//唤醒一下， 感觉如果为了更快，是不是应该刚调用这个就去唤醒，调用函数会带判断是否有东西可以执行，这样效率更高？
            }
        }
        //调用的时候传入的是迭代器吗？
        template<class InputIterator>
        void schedule(InputIterator begin,InputIterator end)
        {
            bool need_tickle=false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin!=eaccess)
                {
                    need_tickle=scheduleNoLock(&*begin)||need_tickle;
                    //这里要begin++吧？
                }
            }
            if(need_tickle)
            {
                tickle();
            }
        }

    protected:
        virtual void tickle();
        
        virtual void idle();
        virtual bool stopping();
        void SetThis();
        bool hasIdleThreads(){return m_idleThreadCount>0;}

    private:
        template<class FiberOrcb>
        bool scheduleNoLock(FiberOrcb fc,int thread)
        {
            bool need_tickle=m_fibers.empty(); //判空 如果是空的返回true，如果非空返回的是false
            //这里的判空逻辑，为什么不放到最后？
            ScheduleTask ft(fc,thread);
            if(ft.fiber||ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }


    private:
        //对调度任务的定义，任务类型可以是协程/函数二选一，并且可指定调度线程。
        struct ScheduleTask
        {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int thread;

            ScheduleTask(Fiber::ptr f,int thr):fiber(f),thread(thr)
            {
            
            }
            ScheduleTask(Fiber::ptr *f,int thr):thread(thr)
            {
                fiber.swap(*f);
            }
            ScheduleTask(std::function<void()> f,int thr):cb(f),thread(thr)
            {
            }
            ScheduleTask():thread(-1){} //要放在stl容器中必须要有默认构造函数，要不然会无法初始化？
            void reset()
            {
                fiber=nullptr;
                cb=nullptr;
                thread=-1;
            }
        };
    private:
        MutexType m_mutex;//互斥锁
        std::vector<Thread::ptr> m_threads;//线程池
        std::list<ScheduleTask> m_fibers; //任务队列? m_tasks  即将要执行/或者说计划要执行的协程
        std::string m_name;//协程调度器名词
        // use_caller为true时，调度器所在线程的调度协程
        Fiber::ptr m_rootFiber;


    protected:
    //多和携程调度器的状态有关
        std::vector<int> m_threadIds;//线程池的线程ID数组
        //工作线程数量，不包含use_caller的主线程
        size_t m_threadCount=0;
        //活跃线程数
        std::atomic<size_t> m_activateThreadCount={0};
        // idle线程数  idle是空闲进程吧?
        std::atomic<size_t> m_idleThreadCount={0};

        //是否use caller
        bool m_useCaller;
        //use_caller为true时，调度器所在线程的id （如果使用use_caller就是主线程id）
        int m_rootThread=-1;
        //是否正在停止  true表示停止
        bool m_stopping =true;
        bool m_autoStop=false;//是否主动停止


    };
}
