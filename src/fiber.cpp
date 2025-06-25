#include "fiber.h"
#include "scheduler.h"
namespace version04
{
    // 全局静态变量，用于生成协程id
    static std::atomic<uint64_t> s_fiber_id{0};
    // 全局静态变量，用于统计当前的协程数
    static std::atomic<uint64_t> s_fiber_count{0};
    // 线程局部变量 当前线程正在运行的协程
    // 保存当前正在运行的协程指针，必须时刻指向当前正在运行的协程对象。协程模块初始化的时候，t_fiber指向线程主协程对象。
    static thread_local Fiber *t_fiber = nullptr;
    // 线程局部变量 当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行，智能指针形式
    /*
    t_thread_fiber：保存线程主协程指针，智能指针形式。
    协程模块初始化时，t_thread_fiber指向线程主协程对象。
    当子协程resume时，通过swapcontext将主协程的上下文保存到t_thread_fiber的ucontext_t成员中，
    同时激活子协程的ucontext_t上下文。
    当子协程yield时，从t_thread_fiber中取得主协程的上下文并恢复运行。
    */
    // static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber = nullptr;
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }
        static void Dealloc(void *vp)
        {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;
    /*
    构造函数，无参数构造只用于创建线程的第一个协程，也就是线程主函数对应的协程，
    这个协程只能由GetThis()方法调用，这个构造函数是私有的
    */
    Fiber::Fiber()
    {
        m_state = State::EXEC;
        SetThis(this);

        // 这个函数不是只有初次建立该线程的协程才会调用吗？
        // 对于主协程也需要调用getcontext获取上下文吗?
        if (getcontext(&m_ctx))
        {
            VERSION04_ASSERT2(false, "getcontext");
        }

        ++s_fiber_count;
        ULOG_DEBUG("main", "Fiber::Fiber");
    }
    /*
        有参构造函数，用于创建子协程
    */
    Fiber::Fiber(std::function<void()> cb, size_t stacksize,bool use_caller):m_id(++s_fiber_id), m_cb(cb)
    {
        ++s_fiber_count;
        nlohmann::json config_json_base = get_config();
        std::string expr = config_json_base["fiber"][0]["stack_size"];
        m_stacksize = stacksize ? stacksize : version04::evaluate_expression(expr);
        m_stacksize=stacksize;
        m_stack = StackAllocator::Alloc(m_stacksize);
        if (getcontext(&m_ctx))
        {
            VERSION04_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        if(!use_caller)
        {

            makecontext(&m_ctx, Fiber::MainFunc, 0);
        }else
        {
            makecontext(&m_ctx,&Fiber::CallerMainFunc,0);
        }

        ULOG_DEBUG("main", "Fiber::Fiber id= {}", m_id);
    }

    Fiber::~Fiber()
    {
        --s_fiber_count;
        if (m_stack)
        {
            VERSION04_ASSERT2(m_state == State::TERM || m_state == State::EXCEPT || m_state == State::INIT,"~fiber()");
            // StackAllocator::Dealloc(m_stack, m_stacksize);
            StackAllocator::Dealloc(m_stack);
        }
        else // 只有主协程没有栈
        {
            // 主协程的m_id应该是0
            VERSION04_ASSERT(!m_cb);
            VERSION04_ASSERT(m_state == State::EXEC);

            Fiber *cur = t_fiber;
            if (cur == this)
            {
                SetThis(nullptr);
            }
        }
        ULOG_DEBUG("main", "Fiber::~Fiber id= {}", m_id);
    }

    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }
    /*
        返回当前线程正在执行的协程
        如果当前线程还未创建协程，则创建线程的第一个协程，创建的第一个协程就是该线程的
        主协程，其他协程都通过这个协程来调度，
        也就是说，其他协程结束时，都要切回主协程，由主协程重新选择新的协程进行resume

        线程如果要创建协程，首先就是执行这个Fiber::GetThis()操作，以初始化主函数协程
    */
    Fiber::ptr Fiber::GetThis()
    {
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        Fiber::ptr main_fiber(new Fiber); // 这个等价于std::shared_ptr main_fiber(new Fiber); //是的 就是调用了无参的构造函数 然后用智能指针管理
        // t_fiber没有用智能指针
        VERSION04_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::MainFunc()
    {
        Fiber::ptr cur = GetThis();
        VERSION04_ASSERT2(cur, "Fiber::MainFunc");
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = State::TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = State::EXCEPT;
            ULOG_ERROR("system", "Fiber Except: {} fiber_id= {}\n", ex.what(), cur->getId());
            VERSION04_ASSERT(false);
        }
        catch (...)
        {

            cur->m_state = State::EXCEPT;
            ULOG_ERROR("system", "Fiber Except, fiber_id= {}\n", cur->getId());
            VERSION04_ASSERT(false);
        }
        auto raw_ptr=cur.get();
        cur.reset();
        raw_ptr->swapOut();
        VERSION04_ASSERT2(false,"never reach fiber_id= "+std::to_string(raw_ptr->getId()));
    }

     void Fiber::CallerMainFunc()
    {
        Fiber::ptr cur = GetThis();
        VERSION04_ASSERT2(cur, "Fiber::MainFunc");
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = State::TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = State::EXCEPT;
            ULOG_ERROR("system", "Fiber Except: {} fiber_id= {}\n", ex.what(), cur->getId());
            VERSION04_ASSERT(false);
        }
        catch (...)
        {

            cur->m_state = State::EXCEPT;
            ULOG_ERROR("system", "Fiber Except, fiber_id= {}\n", cur->getId());
            VERSION04_ASSERT(false);
        }
        auto raw_ptr=cur.get();
        cur.reset();
        raw_ptr->back();
        VERSION04_ASSERT2(false,"never reach fiber_id= "+std::to_string(raw_ptr->getId()));
    }

    // 重置协程函数，并重置状态
    // 当一个线程执行完毕后，为了充分利用这个内存，基于这个内存再去创建一个新的协程
    void Fiber::reset(std::function<void()> cb)
    {
        // 主协程是没有栈的  这里面都是子协程肯定都要是有栈的
        VERSION04_ASSERT(m_stack);
        // 状态要是在结束状态或者init状态才能进行重置
        VERSION04_ASSERT(m_state == State::TERM || m_state == State::INIT || m_state == State::EXCEPT);
        m_cb = cb; // 将回调函数记录
        if (getcontext(&m_ctx))
        {
            VERSION04_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = State::INIT;
    }

    // 看起来是 从主协程-> 当前协程 上下文切换
    void Fiber::call()
    {
        m_state = State::EXEC;
        ULOG_ERROR("system", "{}", getId());
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx))
        {
            VERSION04_ASSERT2(false, "call() swapcontext");
        }
    }
    void Fiber::back()
    {
        SetThis(t_threadFiber.get());
        // m_state=State::READY;
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx))
        {
            VERSION04_ASSERT2(false, "swapout() swapcontext");
        }
    }

    // 将目标唤醒到当前协程执行 将当前正在运行的切换到后台，运行自己
    void Fiber::swapIn()
    {
        SetThis(this);
        VERSION04_ASSERT2(m_state != State::EXEC, "swapIn()");
        m_state = State::EXEC;
        if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx))
        {
            VERSION04_ASSERT2(false, "swapIn() swapcontext");
        }
    }
    void Fiber::swapOut()
    {
        SetThis(t_threadFiber.get());
        // m_state=State::READY;
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx))
        {
            VERSION04_ASSERT2(false, "swapout() swapcontext");
        }
    }

    void Fiber::YieldToReady()
    {
        Fiber::ptr cur = GetThis();
        cur->m_state = State::READY;
        cur->swapOut();
    }
     void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        cur->m_state = State::HOLD;
        cur->swapOut();
    }
    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

    uint64_t Fiber::GetFiberId()
    {
        if(t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }

}