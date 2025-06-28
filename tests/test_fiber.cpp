#include <iostream>
#include "fiber.h"
#include "controllogger.h"
#include "util.h"
// using namespace std;

void run_in_fiber()
{
    ULOG_INFO_SRC("main","run_in_fiber begin");
    // version04::Fiber::YieldToHold();
    // ULOG_INFO_SRC("main","run_in_fiber end");
    // version04::Fiber::YieldToHold();
}


// int main(int argc,char **argv)
// {
//     version04::initLogs();
//     ULOG_INFO_SRC("main","main begin");
//     version04::Fiber::GetThis();
//     version04::Fiber::ptr fiber(new version04::Fiber(run_in_fiber,102400));
//     fiber->swapIn();
//     ULOG_INFO_SRC("main","main after swapIn");
//     fiber->swapIn();
//     ULOG_INFO_SRC("main","main after end");
    
//     return 0;
// }


int main(int argc,char **argv)
{
    version04::initLogs();
    ULOG_INFO_SRC("main","main begin");
    std::shared_ptr<version04::Fiber> fiber;
    fiber.reset(new version04::Fiber(run_in_fiber));
    // fiber->swapIn();
    // ULOG_INFO_SRC("main","main after swapIn");
    // fiber->swapIn();
    // ULOG_INFO_SRC("main","main after end");
    
    return 0;
}