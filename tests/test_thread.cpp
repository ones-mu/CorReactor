#include <unistd.h>
#include "test_thread.h"
#include "controllogger.h"
#include "util.h"

int count=0;
// version04::RWMutex s_mutex;
version04::Mutex s_mutex;

void func1()
{
    // std::cout<<"func1 run"<<std::endl;
//    ULOG_INFO("main","func1 run"); 
   ULOG_INFO("main", "name: {} this.name: {} id: {} this.id: {}",
          version04::Thread::GetName(),
          version04::Thread::GetThis()->getName(),
          version04::GetThreadId(),
          version04::Thread::GetThis()->getId());
    // sleep(20);
    int nums=100000;
    for(int i=0;i<nums; ++i)
    {
        // version04::RWMutex::WriteLock lock(s_mutex);
        version04::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void func2()
{
    while(true)
    ULOG_INFO("main","func2 runxxxxxxxxxxxxxxxxxxxxxxx");
}

void func3()
{
    while(true)
    {
        ULOG_INFO("main","func3 run====================");
    }
}


int main(int argc, char* argv[])
{
    version04::initLogs();
    ULOG_INFO("main","thread test begin");
    std::vector<version04::Thread::ptr> thrs;
    int nums=5;
    for(int i =0; i < nums; ++i) 
    {
        version04::Thread::ptr thr(new version04::Thread(&func1,"name_"+std::to_string(i)));
        version04::Thread::ptr thr2(new version04::Thread(&func2,"name2_"+std::to_string(i*2)));
        version04::Thread::ptr thr3(new version04::Thread(&func3,"name2_"+std::to_string(i*2+1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
        thrs.push_back(thr3);
    }
    for(int i=0; i<thrs.size(); ++i)
    {
        thrs[i]->join();
    }
    // for(int i=0; i<nums; ++i)
    // {
    //     this[i]->
    // }
    ULOG_INFO("main","count: {}",count);
    ULOG_INFO("main","thread test end");
    return 0;
}