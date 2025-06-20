#include "test_thread.h"
#include <unistd.h>


void func1()
{
    
    sleep(20);
}



int main(int argc, char* argv[])
{
    std::vector<version04::Thread::ptr> thrs;
    int nums=0;
    for(int i =0; i < nums; ++i) 
    {
        version04::Thread::ptr thr(new version04::Thread(&func1,"name_"+std::to_string(i)));
        thrs.push_back(thr);
    }
    for(int i=0; i<nums; ++i)
    {
        thrs[i]->join();
    }
    return 0;
}