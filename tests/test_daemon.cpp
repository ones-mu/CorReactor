#include "daemon.h"
#include "controllogger.h"
#include "iomanager.h"
#include "util.h"

version04::Timer::ptr timer;
int server_main(int argc, char* argv[])
{
    ULOG_INFO_SRC("main","{}",version04::ProcessInfoMgr::GetInstance()->toString());
    version04::IOManager iom(1);
    timer = iom.addTimer(1000, []() {
        ULOG_INFO_SRC("main","onTimer");
        static int count = 0;
        if(++count>10)
        {
            timer->cancel();
        }
    },true);
    return 0;
}


int main(int argc, char* argv[])
{
    version04::initLogs();
    return version04::start_daemon(argc,argv,server_main,argc!=1);
}