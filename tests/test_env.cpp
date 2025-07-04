#include "env.h"
#include "controllogger.h"
#include <iostream>

int main(int argc, char **argv)
{
    version04::initLogs();
    ULOG_INFO_SRC("main", "argc={}", argc);
    version04::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    version04::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    version04::EnvMgr::GetInstance()->addHelp("p", "print help");
    if (!version04::EnvMgr::GetInstance()->init(argc, argv))
    {
        version04::EnvMgr::GetInstance()->printHelp();
        return 0;
    }
    if(version04::EnvMgr::GetInstance()->has("p")) {
        version04::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}