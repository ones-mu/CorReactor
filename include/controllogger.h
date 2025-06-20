#pragma once
#include <mutex>
#include <unordered_map>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;



// 便捷日志宏
#define ULOG_TRACE(logger, ...)    UltimateLogger::instance().log(UltimateLogger::Level::TRACE, logger, __VA_ARGS__)
#define ULOG_DEBUG(logger, ...)    UltimateLogger::instance().log(UltimateLogger::Level::DEBUG, logger, __VA_ARGS__)
#define ULOG_INFO(logger, ...)     UltimateLogger::instance().log(UltimateLogger::Level::INFO, logger, __VA_ARGS__)
#define ULOG_WARN(logger, ...)     UltimateLogger::instance().log(UltimateLogger::Level::WARN, logger, __VA_ARGS__)
#define ULOG_ERROR(logger, ...)    UltimateLogger::instance().log(UltimateLogger::Level::ERROR, logger, __VA_ARGS__)
#define ULOG_CRITICAL(logger, ...) UltimateLogger::instance().log(UltimateLogger::Level::CRITICAL, logger, __VA_ARGS__)

// 带源文件位置的日志宏
#define ULOG_TRACE_SRC(logger, ...)    UltimateLogger::instance().log(UltimateLogger::Level::TRACE, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_DEBUG_SRC(logger, ...)    UltimateLogger::instance().log(UltimateLogger::Level::DEBUG, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_INFO_SRC(logger, ...)     UltimateLogger::instance().log(UltimateLogger::Level::INFO, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_WARN_SRC(logger, ...)     UltimateLogger::instance().log(UltimateLogger::Level::WARN, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_ERROR_SRC(logger, ...)    UltimateLogger::instance().log(UltimateLogger::Level::ERROR, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_CRITICAL_SRC(logger, ...) UltimateLogger::instance().log(UltimateLogger::Level::CRITICAL, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))


class ControlLogger
{
    // friend class config;

public:
    // 日志输出模式
    enum class OutputMode
    {
        SYNC = 0, // 同步模式
        ASYNC = 1 // 异步模式
    };
    // 日志级别枚举
    enum class Level
    {
        TRACE = spdlog::level::trace,
        DEBUG = spdlog::level::debug,
        INFO = spdlog::level::info,
        WARN = spdlog::level::warn,
        ERROR = spdlog::level::err,
        CRITICAL = spdlog::level::critical,
        OFF = spdlog::level::off
    };
    //禁止移动和拷贝构造函数
    ControlLogger(const ControlLogger&) = delete;
    ControlLogger& operator=(const ControlLogger&) = delete;
    ControlLogger(ControlLogger&&) = delete;
    ControlLogger& operator=(ControlLogger&&) = delete;


     // 初始化配置
    struct Config {
        OutputMode mode = OutputMode::SYNC;
        std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v";
        std::chrono::seconds flush_interval = std::chrono::seconds(3);
        Level flush_level = Level::WARN;
        size_t async_queue_size = 8192;
    };

    //获取单例实例
    static ControlLogger& instance();
    //初始化(线程安全)
    bool initialize(const Config& config);
    bool initializeFromFile(const std::string& config_path);
    //日志记录接口
    template<typename... Args>
    void log(Level level,const std::string& logger_name,Args&&... args)
    {
        auto logger=getLogger(logger_name);
        if(logger)
        {
            logger->log(static_cast<spdlog::level::level_enum>(level),
                        fmt::format(std::forward<Args>(args)...));
        }
    }

private:
    ControlLogger() = default;
    ~ControlLogger();

    std::shared_ptr<spdlog::logger> getLogger(const std::string& logger_name);
    void setupAsyncMode();
    void setupSyncMode();

    //成员变量
    std::mutex mutex_;
    bool initialized_ = false;
    Config config_;
    std::shared_ptr<spdlog::details::thread_pool> async_thread_pool_;
    std::unordered_map<std::string,std::shared_ptr<spdlog::logger>> loggers_;
};