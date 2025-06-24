// json_utils.hpp
#pragma once

#include "nlohmann/json.hpp"
#include <fstream>
#include <stdexcept> // for std::runtime_error

namespace version04
{ // 可选命名空间

    inline nlohmann::json read_json_base(const std::string &config_path)
    {
        std::ifstream ifs(config_path);
        if (!ifs.is_open())
        {
            throw std::runtime_error("Failed to open config file: " + config_path);
        }

        try
        {
            return nlohmann::json::parse(ifs); // 直接解析，避免中间变量
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("JSON parse error in " + config_path + ": " + e.what());
        }
    }

    // nlohmann::json config_json_base;

    // void read_json_global()
    // {
    //     const std::string config_path="config/configBase.json";
    //     config_json_base=read_json_base(config_path);
    // }

    //​​Meyer's Singleton​​（线程安全单例）

    inline const nlohmann::json& get_config()
    {
        static const nlohmann::json instance=[](){

            const std::string config_path="config/configBase.json";
            return read_json_base(config_path);
        }(); //在定义后加()  表示立即调用
        return instance;
    }



} // namespace version04