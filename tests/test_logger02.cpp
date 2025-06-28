#include "ultimate_logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

int main() {
    // 1. 初始化日志系统
    if (!UltimateLogger::instance().initializeFromFile("config/config.json")) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    // 2. 可选：读取配置文件内容用于日志输出
    nlohmann::json config_json;
    try {
        std::ifstream config_file("config/config.json");
        config_file >> config_json;
    } catch (const std::exception& e) {
        ULOG_ERROR_SRC("main", "Failed to read config file: {}", e.what());
    }

    // 3. 记录日志
    ULOG_INFO_SRC("main", "Application started");
    ULOG_DEBUG_SRC("main", "Initializing with config: {}", config_json.dump(2)); // 使用缩进美化输出

    // 4. 动态修改日志级别
    UltimateLogger::instance().setLevel("main", UltimateLogger::Level::WARN);
    ULOG_DEBUG_SRC("main", "This debug message will not show after level change"); // 这条不会显示

    // 5. 切换同步/异步模式
    UltimateLogger::instance().toggleAsync(false);
    ULOG_INFO_SRC("main", "Switched to sync mode");

    return 0;
}