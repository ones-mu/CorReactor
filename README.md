# CorReactor高性能服务器框架

本项目是以协程为主体的c++高性能服务器框架，其中封装了c中的有栈协程ucontext_t(Fiber类)、和c++20中的协程（无栈协程）(Task类)

项目中多处使用了模版元编程


## TODO

注释风格调整为doxygen风格

更多的使用模版元编程来替代大部分的宏（尤其是日志模块中对spdlog封装的部分），以增强维护性。

添加中间件，以拓宽应用场景

添加ssl、http2.0

maybe:用io_uring(liburing)重构代码以支持更高的并发，io_uring是真正的异步io，但最好不要跨线程操作，这意味着每个处理io事件的都应该维护一个io_uring，这也意味着这个项目的架构要有很大的改动。

___
## introduce

（1）封装了[spdlog](https://github.com/gabime/spdlog)库作为日志模块
使用示例
```cpp
ULOG_INFO_SRC("main", "Fiber::Fiber id= {}, stacksize= {}", m_id, m_stacksize);
```
(2)使用yaml作为配置文件格式（yaml的读取使用yaml-cpp库），封装为配置模块。采用的是约定大于配置，声明一个参数的时候已经设置好了里面的一些默认参数。
支持基础类型如int float string，也支持stl容器类型如vector set map等，也可以使用自定义格式，但需要手动实现自定义格式到string之间的相互转换。
支持热加载，即运行时更新、自动检测变化、通知机制。

(3)封装线程池，各类锁

(4)协程调度器模块(scheduler)，为非对称式调度,是N:M协程调度器

(5)将协程调度器与 epoll(边沿触发模式) 和定时器模块(TimerManage)深度整合，创建了核心的 IO协程调度器(如 IOManager)。通过设计​​空闲协程陷入epoll_wait​​的机制，并利用 IO事件、定时器事件和管道通知等方式​​唤醒阻塞线程进行高效调度​​，最大化CPU利用率。
支持IO事件和定时事件的注册和回调。

(6)协程化 Hook，Socket API(connect, accept, recv, send等)、sleep系列函数及fd操作相关阻塞性系统调用进行了 ​​异步非阻塞封装 (Hook)​​。使得可以以同步的方式进行调用，但又有异步的性能。

(7)对socket套接字、tcp\udp传输层等模块进行了封装、仅支持Http1.1，还不支持Https（http协议的解析式基于Ragel（有限状态机））

(8)有守护进程

补充：
这个version04不是主从reactor的形式，因为全局仅有一个epoll(在超高并发量下可能会成为性能瓶颈)。

支持定时事件的添加、删除、更新。可以指定那个协程任务必须在那个线程下工作。

协程队列是用的std::list（另：高性能的多生产者-多消费无锁环形缓冲队列,如[atomic_queue](https://github.com/max0x7ba/atomic_queue)也非常适合），全线程共享，FIFO,拿去放入靠线程锁，线程轮训主动拿取，因此不需要负载均衡。

## version 04
主体架构来自sylar老哥的项目[C++高性能分布式服务器框架](https://github.com/sylar-yin/sylar)
部分有所不同

## version 01-03
前几个版本是主从reactor
epoll..
线程池
另外还封装了poll、select
