#include <unistd.h>
#include "test_thread.h"
#include "controllogger.h"
#include "util.h"

int count=0;
version04::RWMutex s_mutex;

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
        version04::RWMutex::WriteLock lock(s_mutex);
        ++count;
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
        thrs.push_back(thr);
    }
    for(int i=0; i<nums; ++i)
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