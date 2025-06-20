#include "controllogger.h"

int main(void)
{
    // 方式1: 代码配置
    // ControlLogger::Config config;
    // config.mode = ControlLogger::OutputMode::ASYNC;
    // config.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
    // ControlLogger::instance().initialize(config);

    // // 添加输出目标
    // ControlLogger::instance().addConsoleSink("main", ControlLogger::Level::DEBUG);
    // ControlLogger::instance().addRotatingFileSink("main", "logs/app.log",
    //                                                ControlLogger::Level::INFO, 1024 * 1024 * 5, 3);

    // 方式2: 从文件加载配置
    ControlLogger::instance().initializeFromFile("config.json");

    //记录日志
    ULOG_INFO

    return 0;
}