#pragma once
#include <mutex>
#include <unordered_map>
#include <functional>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include "nlohmann/json.hpp"
#include "util.h"
using json = nlohmann::json;

// 便捷日志宏
#define ULOG_TRACE(logger, ...) ControlLogger::instance().log(ControlLogger::Level::TRACE, logger, __VA_ARGS__)
#define ULOG_DEBUG(logger, ...) ControlLogger::instance().log(ControlLogger::Level::DEBUG, logger, __VA_ARGS__)
#define ULOG_INFO(logger, ...) ControlLogger::instance().log(ControlLogger::Level::INFO, logger, __VA_ARGS__)
#define ULOG_WARN(logger, ...) ControlLogger::instance().log(ControlLogger::Level::WARN, logger, __VA_ARGS__)
#define ULOG_ERROR(logger, ...) ControlLogger::instance().log(ControlLogger::Level::ERROR, logger, __VA_ARGS__)
#define ULOG_CRITICAL(logger, ...) ControlLogger::instance().log(ControlLogger::Level::CRITICAL, logger, __VA_ARGS__)
/*
#define ULOG_TRACE(logger, ...) \
    ControlLogger::instance().log(ControlLogger::Level::TRACE, logger, \
        "[Thread:{}][Fiber:{}] " __VA_ARGS__, version04::GetThreadId(), version04::GetFiberId())

#define ULOG_DEBUG(logger, ...) \
    ControlLogger::instance().log(ControlLogger::Level::DEBUG, logger, \
        "[Thread:{}][Fiber:{}] " __VA_ARGS__, version04::GetThreadId(), version04::GetFiberId())

#define ULOG_INFO(logger, ...) \
    ControlLogger::instance().log(ControlLogger::Level::INFO, logger, \
        "[Thread:{}][Fiber:{}] " __VA_ARGS__, version04::GetThreadId(), version04::GetFiberId())

#define ULOG_WARN(logger, ...) \
    ControlLogger::instance().log(ControlLogger::Level::WARN, logger, \
        "[Thread:{}][Fiber:{}] " __VA_ARGS__, version04::GetThreadId(), version04::GetFiberId())
#define ULOG_ERROR(logger, ...) \
    ControlLogger::instance().log(ControlLogger::Level::ERROR, logger, \
        "[Thread:{}][Fiber:{}] " __VA_ARGS__, version04::GetThreadId(), version04::GetFiberId())

#define ULOG_CRITICAL(logger, ...) \
    ControlLogger::instance().log(ControlLogger::Level::CRITICAL, logger, \
        "[Thread:{}][Fiber:{}] " __VA_ARGS__, version04::GetThreadId(), version04::GetFiberId())
*/
// 带源文件位置的日志宏
#define ULOG_TRACE_SRC(logger, ...) ControlLogger::instance().log(ControlLogger::Level::TRACE, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_DEBUG_SRC(logger, ...) ControlLogger::instance().log(ControlLogger::Level::DEBUG, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_INFO_SRC(logger, ...) ControlLogger::instance().log(ControlLogger::Level::INFO, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_WARN_SRC(logger, ...) ControlLogger::instance().log(ControlLogger::Level::WARN, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_ERROR_SRC(logger, ...) ControlLogger::instance().log(ControlLogger::Level::ERROR, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define ULOG_CRITICAL_SRC(logger, ...) ControlLogger::instance().log(ControlLogger::Level::CRITICAL, logger, "{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))

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
    // 禁止移动和拷贝构造函数
    ControlLogger(const ControlLogger &) = delete;
    ControlLogger &operator=(const ControlLogger &) = delete;
    ControlLogger(ControlLogger &&) = delete;
    ControlLogger &operator=(ControlLogger &&) = delete;

    // 初始化配置
    struct Config
    {
        OutputMode mode = OutputMode::SYNC;
        std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v";
        std::chrono::seconds flush_interval = std::chrono::seconds(3);
        Level flush_level = Level::WARN;
        size_t async_queue_size = 8192;
    };

    // 获取单例实例
    static ControlLogger &instance();
    // 初始化(线程安全)
    bool initialize(const Config &config);
    bool initializeFromFile(const std::string &config_path);
    // 日志记录接口
    // template <typename... Args>
    // void log(Level level, const std::string &logger_name, Args &&...args)
    // {
    //     if (!initialized_)
    //         return;
    //     auto logger = getLogger(logger_name);
    //     if (logger)
    //     {
    //         logger->log(static_cast<spdlog::level::level_enum>(level),
    //                     std::forward<Args>(args)...);
    //     }
    // }
    template <typename... Args>
    void log(Level level, const std::string &logger_name,
             fmt::format_string<Args...> fmt, Args &&...args)
    {
        if (!initialized_)
            return;
        auto logger = getLogger(logger_name);
        if (logger)
        {
            // 添加线程和纤程ID前缀
            auto full_msg = fmt::format("[Thread:{}][Fiber:{}] {}", 
                                  version04::GetThreadId(), 
                                  version04::GetFiberId(),
                                  fmt::format(fmt,std::forward<Args>(args)...));
            // logger->log(static_cast<spdlog::level::level_enum>(level),
            //             fmt, std::forward<Args>(args)...);
             logger->log(static_cast<spdlog::level::level_enum>(level), full_msg);
        }
    }
    // 添加输出目标
    bool addConsoleSink(const std::string &logger_name, Level level);
    bool addRotatingFileSink(const std::string &logger_name, const std::string &filepath,
                             Level level, size_t max_size, size_t max_files);
    bool addDailyFileSink(const std::string &logger_name, const std::string &filepath,
                          Level level, int hour = 0, int minute = 0);

    // 动态控制
    void setLevel(const std::string &logger_name, Level level);
    void setPattern(const std::string &pattern);
    void toggleAsync(bool enable);
    void flushAll();

private:
    ControlLogger() = default;
    ~ControlLogger();

    std::shared_ptr<spdlog::logger> getLogger(const std::string &logger_name);
    void registerLogger(const std::string &name, const std::vector<spdlog::sink_ptr> &sinks);
    void setupAsyncMode();
    void setupSyncMode();

    // 成员变量
    std::mutex mutex_;
    bool initialized_ = false;
    Config config_;
    std::shared_ptr<spdlog::details::thread_pool> async_thread_pool_;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
    std::vector<spdlog::sink_ptr> global_sinks_;
};