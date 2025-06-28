#include "util.h"
#include "scheduler.h"
#include "controllogger.h"

void test_print();
void test_fiber()
{
    static int s_count=5;
    ULOG_INFO("main","test in fiber s_count= {}",s_count);
    sleep(1);
    if(--s_count>=0)
    {
        
        ULOG_INFO("main","0222test in fiber s_count= {}",s_count);
        version04::Scheduler::GetThis()->schedule(&test_fiber,version04::GetThreadId());//还是选用这个线程执行
        // version04::Scheduler::GetThis()->schedule(&test_fiber);
    }
}

void test_print()
{
    static int count=0;
    ULOG_INFO("main","test in print count= {}",count);
    sleep(1);
}


int main(void)
{
    version04::initLogs();
    ULOG_INFO("main","test scheduler main begin");
    version04::Scheduler sc(3,false,"first_scheduler");
    // version04::Scheduler sc(4,true,"first_scheduler");
    sc.start();
    ULOG_INFO("main","schedule");
    sleep(1);
    sc.schedule(&test_fiber);
    sc.stop();
    // while(1)
    // {
    //     //不想让主线程退出
    // }
    ULOG_INFO("main","over");
    ULOG_INFO("main","test scheduler main end");
    return 0;
}