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

        const std::string& getExe() const { return m_exe; }
        const std::string& getCwd() const { return m_cwd; }

        bool setEnv(const std::string &key, const std::string &val);
        std::string getEnv(const std::string &key, const std::string &default_val = "");

        std::string getAbsolutePath(const std::string& path) const;

    private:
        RWMutexType m_mutex;
        std::map<std::string, std::string> m_args;
        std::vector<std::pair<std::string, std::string>> m_helps;

        std::string m_program;

        std::string m_exe;//绝对的路径
        std::string m_cwd;//可执行文件的路径
    };

    using EnvMgr = version04::Singleton<Env>;
}