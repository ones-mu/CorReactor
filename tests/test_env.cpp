#include "env.h"
#include "controllogger.h"
#include <iostream>
#include <fstream>

struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for (size_t i = 0; i < content.size(); ++i)
        {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

A a;

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

    ULOG_INFO_SRC("main", "exe={}", version04::EnvMgr::GetInstance()->getExe());
    ULOG_INFO_SRC("main", "cwd={}", version04::EnvMgr::GetInstance()->getCwd());
    std::cout << "exe=" << version04::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << version04::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << version04::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << version04::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    // std::cout << "set env " << version04::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    // std::cout << "test=" << version04::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;

    if (version04::EnvMgr::GetInstance()->has("p"))
    {
        version04::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}