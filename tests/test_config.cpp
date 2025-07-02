#include "config.h"
#include "controllogger.h"
#include "util.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

version04::ConfigVar<int>::ptr g_int_value_config = version04::Config::Lookup<int>("system.port", (int)8080, "system port");
version04::ConfigVar<float>::ptr g_float_value_config = version04::Config::Lookup<float>("system.value", (float)3.14, "system value");

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

int main(void)
{
    version04::initLogs();
    test_yaml();
    // ULOG_INFO_SRC("main","port is {}",g_int_value_config->getValue());
    // ULOG_INFO_SRC("main","port is {}",g_int_value_config->toString());
    // ULOG_INFO_SRC("main","port is {}",g_float_value_config->getValue());
    // ULOG_INFO_SRC("main","port is {}",g_float_value_config->toString());

    return 0;
}