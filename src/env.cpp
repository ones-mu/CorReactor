#include "env.h"
#include "controllogger.h"

#include <cstring>
#include <iostream>
#include <iomanip> //setw
#include <unistd.h>
#include <stdlib.h>

namespace version04
{
    bool Env::init(int argc, char **argv)
    {

        // 要读取他链接的地址
        // 将软链接转成绝对路径 通过readlink
        char link[1024] = {0}; // 链接的位置
        char path[1024] = {0}; // 真正的位置
        sprintf(link, "/proc/%d/exe", getpid());
        readlink(link, path, sizeof(path));
        // readlink(link, path, sizeof(path) - 1);
        // /path/xxx/exe
        m_exe = path;

        auto pos = m_exe.find_last_of("/");
        m_cwd = m_exe.substr(0, pos) + "/";

        m_program = argv[0];
        // -config /path/to/config  -file xxxx
        const char *now_key = nullptr;
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i][0] == '-')
            {
                if (strlen(argv[i]) > 1)
                {
                    if (now_key)
                    {
                        add(now_key, "");
                    }
                    now_key = argv[i] + 1;
                }
                else
                {
                    ULOG_ERROR_SRC("main", "invalid arg idx={},val={}", i, argv[i]);
                    return false;
                }
            }
            else
            {
                if (now_key)
                {
                    add(now_key, argv[i]);
                    now_key = nullptr;
                }
                else
                {
                    ULOG_ERROR_SRC("main", "invalid arg idx={},val={}", i, argv[i]);
                    return false;
                }
            }
        }
        if (now_key)
        {
            add(now_key, "");
        }
        return true;
    }
    void Env::add(const std::string &key, const std::string &val)
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_args[key] = val;
    }
    bool Env::has(const std::string &key)
    {
        RWMutexType::ReadLock lock(m_mutex);
        return m_args.find(key) != m_args.end();
    }
    void Env::del(const std::string &key)
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_args.erase(key);
    }
    std::string Env::get(const std::string &key, const std::string &default_val)
    {
        RWMutexType::ReadLock lock(m_mutex);
        return m_args.find(key) != m_args.end() ? m_args[key] : default_val;
    }
    void Env::addHelp(const std::string &key, const std::string &desc)
    {
        removeHelp(key);
        RWMutexType::WriteLock lock(m_mutex);
        m_helps.emplace_back(key, desc);
    }
    void Env::removeHelp(const std::string &key)
    {
        RWMutexType::WriteLock lock(m_mutex);
        for (auto it = m_helps.begin(); it != m_helps.end(); ++it)
        {
            if (it->first == key)
            {
                m_helps.erase(it);
                break;
            }
        }
    }
    void Env::printHelp()
    {
        RWMutexType::ReadLock lock(m_mutex);
        // 将使用的正确方式给输出出来
        // std::cout << "Usage: " << m_program << " [options]" << std::endl;
        // for (auto &i : m_helps)
        // {
        //     std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
        // }

        ULOG_INFO_SRC("main", "Usage: {} [options]", m_program);
        for (auto &i : m_helps)
        {
            ULOG_INFO_SRC("main", "-{} : {}", i.first, i.second);
        }
    }
    bool Env::setEnv(const std::string &key, const std::string &val)
    {
        return !setenv(key.c_str(), val.c_str(), 1);
    }

    std::string Env::getEnv(const std::string &key, const std::string &default_value)
    {
        const char *v = getenv(key.c_str());
        if (v == nullptr)
        {
            return default_value;
        }
        return v;
    }
    std::string Env::getAbsolutePath(const std::string &path) const
    {
        if (path.empty())
        {
            return "/";
        }
        if (path[0] == '/')
        {
            return path;
        }
        return m_cwd + path;
    }
}