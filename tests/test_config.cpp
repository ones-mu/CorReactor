#include "config.h"
#include "controllogger.h"
#include "util.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <vector>
#include <string>

version04::ConfigVar<int>::ptr g_int_value_config = version04::Config::Create<int>("system.port", (int)8080, "system port");
version04::ConfigVar<float>::ptr g_float_value_config = version04::Config::Create<float>("system.value", (float)3.14, "system value");
version04::ConfigVar<std::vector<int>>::ptr g_vector_value_config = version04::Config::Create<std::vector<int>>("system.name", std::vector<int>{1, 2, 3}, "system name");
// void print_yaml(const YAML::Node& node, int level) {
//     if(node.IsScalar()) {
//         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
//             << node.Scalar() << " - " << node.Type() << " - " << level;
//     } else if(node.IsNull()) {
//         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
//             << "NULL - " << node.Type() << " - " << level;
//     } else if(node.IsMap()) {
//         for(auto it = node.begin();
//                 it != node.end(); ++it) {
//             SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
//                     << it->first << " - " << it->second.Type() << " - " << level;
//             print_yaml(it->second, level + 1);
//         }
//     } else if(node.IsSequence()) {
//         for(size_t i = 0; i < node.size(); ++i) {
//             SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
//                 << i << " - " << node[i].Type() << " - " << level;
//             print_yaml(node[i], level + 1);
//         }
//     }
// }

class Person
{
public:
    Person() {};
    std::string m_name ="";
    int m_age = 0;
    bool m_sex = 0;
    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex << "]";
        return ss.str();
    }
    bool operator==(const Person &other) const
    {
        return m_name == other.m_name && m_age == other.m_age && m_sex == other.m_sex;
    }
};

namespace version04
{

    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

}

version04::ConfigVar<Person>::ptr g_person = version04::Config::Create("class.person", Person(), "system person");
version04::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    version04::Config::Create("class.map", std::map<std::string, Person>(), "system person");

void test_class()
{
    ULOG_INFO_SRC("main", "before person is {}-{}", g_person->getValue().toString(), g_person->toString());
    YAML::Node root = YAML::LoadFile("/home/wsl2_ubuntu_2204/workspace/c_workation/ReactorWebServer/conf/test.yml");
    version04::Config::LoadFromYaml(root);
    ULOG_INFO_SRC("main", "afte person is {}-{}", g_person->getValue().toString(), g_person->toString());
}

void printYamlNode(const YAML::Node &node, const std::string &prefix = "")
{
    if (node.IsMap())
    {
        for (const auto &entry : node)
        {
            std::string key = entry.first.as<std::string>();
            spdlog::info("{}{}:", prefix, key);
            printYamlNode(entry.second, prefix + "  ");
        }
    }
    else if (node.IsSequence())
    {
        int index = 0;
        for (const auto &entry : node)
        {
            spdlog::info("{}- [{}]", prefix, index++);
            printYamlNode(entry, prefix + "  ");
        }
    }
    else
    {
        spdlog::info("{}{}", prefix, YAML::Dump(node));
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/wsl2_ubuntu_2204/workspace/c_workation/ReactorWebServer/conf/log.yml");
    // ULOG_INFO_SRC("main","test yaml {}",root);
    // std::cout<<root<<std::endl;
    // printYamlNode(root);
    // 将 YAML 节点转换为字符串并打印
    spdlog::info("YAML content:\n{}", YAML::Dump(root));

    // 如果你想以更结构化的方式打印
    for (const auto &entry : root)
    {
        spdlog::info("Key: {}, Value: {}", entry.first.as<std::string>(), YAML::Dump(entry.second));
    }
}

void test_config()
{
    ULOG_INFO_SRC("main", "port is {}", g_int_value_config->getValue());
    ULOG_INFO_SRC("main", "port is {}", g_int_value_config->toString());
    ULOG_INFO_SRC("main", "value is {}", g_float_value_config->getValue());
    ULOG_INFO_SRC("main", "value is {}", g_float_value_config->toString());
    auto &v = g_vector_value_config->getValue();
    for (auto &i : v)
    {
        ULOG_INFO_SRC("main", "name is {}", i);
    }
    YAML::Node root = YAML::LoadFile("/home/wsl2_ubuntu_2204/workspace/c_workation/ReactorWebServer/conf/test.yml");
    version04::Config::LoadFromYaml(root);
    ULOG_INFO_SRC("main", "port is {}", g_int_value_config->getValue());
    ULOG_INFO_SRC("main", "port is {}", g_int_value_config->toString());
    ULOG_INFO_SRC("main", "value is {}", g_float_value_config->getValue());
    ULOG_INFO_SRC("main", "value is {}", g_float_value_config->toString());
}

int main(void)
{
    version04::initLogs();
    // test_yaml();
    // test_config();
    test_class();

    return 0;
}