#include "util.h"
#include "scheduler.h"
#include "controllogger.h"

void test_fiber()
{
    static int s_count=5;
    ULOG_INFO("main","test in fiber s_count= {}",s_count);
}


int main(void)
{
    version04::initLogs();
    ULOG_INFO("main","test scheduler main begin");
    version04::Scheduler sc(2,true,"first_scheduler");
    sc.start();
    sc.schedule(&test_fiber);
    // sc.stop();
    while(1)
    {
        //不想让主线程退出
    }
    ULOG_INFO("main","test scheduler main end");
    return 0;
}