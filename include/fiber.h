#pragma once

#include <memory> // std::enable_shared_from_this的头文件也在这
#include <functional>
#include <ucontext.h>
#include <atomic>
#include "macro.h"
#include "controllogger.h"
#include "read_config.h"
#include "util.h"
// #include "scheduler.h"
namespace version04
{
    class Scheduler;
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        public:
            friend class Scheduler; //为了可以在Scheduler对当前类private和protected中的变量进行访问
            using ptr=std::shared_ptr<Fiber>;

            enum class State{
                INIT, //初始状态
                HOLD, //暂停/保持状态
                EXEC, //执行状态
                TERM, //终止状态
                READY, //就绪状态
                EXCEPT //异常状态
            };
        private:
            Fiber();  //不是说使用了enable_shared_from_this之后  构造函数要是public 或是protected吗
        
        public:
            Fiber(std::function<void()> cb,size_t stacksize=0,bool use_caller=false);
            ~Fiber();
            
            //重置协程函数、并重置状态
            //INIT、TERM
            void reset(std::function<void()> cb);
            //切换到当前协程执行
            void swapIn();
            //切换到后台执行
            void swapOut();

            void call();
            void back();

            uint64_t getId() const {return m_id;}

            State getState() const {return m_state;}
        public:
            //设置当前协程
            static void SetThis(Fiber* f);
            //返回当前协程
            static Fiber::ptr GetThis();
            //协程切换到后台，并且设置尾Ready状态
            static void YieldToReady();
            //协程切换到后台，并且设置为Hold状态
            static void YieldToHold();
            //总协程数
            static uint64_t TotalFibers();

            static void MainFunc();
            static void CallerMainFunc();
            static uint64_t GetFiberId();

        
        private:
            uint64_t m_id=0;//协程id
            uint32_t m_stacksize=0;//协程栈大小
            State m_state=State::INIT;//协程状态

            ucontext_t m_ctx;//协程上下文
            void* m_stack=nullptr;//协程栈地址

            std::function<void()> m_cb;//协程入口函数
    };
}