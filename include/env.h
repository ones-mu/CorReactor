#pragma once

#include "singleton.h"
#include "thread.h"

#include <map>
#include <vector>
#include <string>

namespace version04
{
    class Env
    {
    public:
        using RWMutexType=version04::RWMutex;
        bool init(int argc, char **argv);
        void add(const std::string &key, const std::string &val);
        bool has(const std::string &key);
        void del(const std::string &key);
        std::string get(const std::string &key,const std::string &default_val="");//取不到的话返回默认值
        void addHelp(const std::string &key, const std::string &desc);
        void removeHelp(const std::string &key);
        void printHelp();
    private:
        RWMutexType m_mutex;
        std::map<std::string, std::string> m_args;
        std::vector<std::pair<std::string, std::string>> m_helps;

        std::string m_program;
    };

    using EnvMgr = version04::Singleton<Env>;
}