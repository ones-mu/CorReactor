#include "config.h"
#include "env.h"

#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace version04
{
    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        auto it = s_datas.find(name);
        return it != s_datas.end() ? it->second : nullptr;
    }
    //"A.B", 10
    // 上面这个在yaml中的话
    // A:
    //   B: 10
    //   C: str
    // 将树结构 弄成list结构
    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
        {
            ULOG_ERROR_SRC("system", "Config invalid name: {}\n{}", prefix, YAML::Dump(node));
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ListAllMember(prefix.empty() ? it->first.Scalar()
                                             : prefix + "." + it->first.Scalar(),
                              it->second, output);
            }
        }
    }
    // Config::ConfigVarMap Config::s_datas;
    // Config::RWMutexType Config::s_mutex;
    void Config::LoadFromYaml(const YAML::Node &root)
    {
        // 将所有root中所有节点信息保存到all_nodes中
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);
        // 遍历all_nodes，将节点信息保存到ConfigVarMap中
        for (auto &i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            // 查找名为key的配置参数
            ConfigVarBase::ptr var = LookupBase(key);
            // 若找到
            if (var)
            { // 若为纯量，则调用fromString（会调用setValue设值）
                if (i.second.IsScalar())
                {
                    var->fromString(i.second.Scalar());
                    // 否则为数组，将其转换为字符串
                }
                else
                {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    static std::map<std::string, uint64_t> s_file2modifytime;
    static version04::Mutex s_mutex_out;

    void Config::LoadFromConfDir(const std::string &path)
    {
        std::string absoulite_path = version04::EnvMgr::GetInstance()->getAbsolutePath(path);
        ULOG_INFO_SRC("main", "Config load dir = {}", absoulite_path);
        std::vector<std::string> files;
        version04::FSUtil::ListAllFile(files, absoulite_path, ".yml");
        for (auto &file : files)
        {
            //看时间戳，时间戳没修改就不改，进阶的话用md5
            struct stat st;
            lstat(file.c_str(), &st);
            {
                version04::Mutex::Lock lock(s_mutex_out);
                if (s_file2modifytime[file] == (uint64_t)st.st_mtime)
                {
                    continue;
                }
                s_file2modifytime[file] = st.st_mtime;
            }
            try
            {
                YAML::Node root = YAML::LoadFile(file);
                LoadFromYaml(root);
                ULOG_INFO_SRC("main", "Config load file {}", file);
            }
            catch (const YAML::Exception &e)
            {
                ULOG_ERROR_SRC("system", "Config load file {} failed: {}", file, e.what());
            }
        }
    }
}