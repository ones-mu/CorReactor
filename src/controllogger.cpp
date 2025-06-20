#include "controllogger.h"
#include <fstream>

ControlLogger &ControlLogger::instance()
{
    static ControlLogger logger;
    return logger;
}

bool ControlLogger::initialize(const Config &config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_)
    {
        return false;
    }
    config_ = config;

    try
    {
        spdlog::set_pattern(config_.pattern);
        spdlog::flush_every(config_.flush_interval);
        spdlog::flush_on(static_cast<spdlog::level::level_enum>(config_.flush_level));
        if (config_.mode == OutputMode::ASYNC)
        {
            setupAsyncMode();
        }
        else
        {
            setupSyncMode();
        }
        initialized_ = true;
        return true;
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        fprintf(stderr, "Logger initialization failed: %s\n", ex.what());
        return false;
    }
}

bool ControlLogger::initializeFromFile(const std::string &config_path)
{
    try
    {
        std::ifstream ifs(config_path);
        if (!ifs.is_open())
        {
            throw std::runtime_error("Failed to open config file: " + config_path);
        }
        json config_json;
        ifs >> config_json;
        ifs.close();
        Config config;
        // 这里面使用了nlohmann/json库的value方法，第一个参数是要查找的键值，第二个参数值是默认值，如果mode键不存在就返回这个值
        config.mode = config_json.value("mode", "SYNC") == "ASYNC" ? OutputMode::ASYNC : OutputMode::SYNC;
        config.pattern = config_json.value("pattern",
                                           "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v");
        config.flush_interval = std::chrono::seconds(
            config_json.value("flush_interval", 3));
        config.async_queue_size = config_json.value("async_queue_size", 8192);

        // 解析日志级别
        auto parse_level = [](const std::string &level_str) -> Level
        {
            static const std::unordered_map<std::string, Level> level_map = {
                {"TRACE", Level::TRACE},
                {"DEBUG", Level::DEBUG},
                {"INFO", Level::INFO},
                {"WARN", Level::WARN},
                {"ERROR", Level::ERROR},
                {"CRITICAL", Level::CRITICAL},
                {"OFF", Level::OFF}};
            auto it = level_map.find(level_str);
            return it != level_map.end() ? it->second : Level::INFO;
        };

        config.flush_level = parse_level(config_json.value("flush_level", "WARN"));

        // 解析日志器和sinks
        for (const auto &logger_json : config_json["loggers"])
        {
            std::string logger_name = logger_json["name"];

            for (const auto &sink_json : logger_json["sinks"])
            {
                std::string type = sink_json["type"];
                Level level = parse_level(sink_json["level"]);

                if (type == "console")
                {
                    addConsoleSink(logger_name, level);
                }
                else if (type == "rotating_file")
                {
                    addRotatingFileSink(
                        logger_name,
                        sink_json["filepath"],
                        level,
                        sink_json.value("max_size", 1024 * 1024 * 5),
                        sink_json.value("max_files", 3));
                }
                else if (type == "daily_file")
                {
                    addDailyFileSink(
                        logger_name,
                        sink_json["filepath"],
                        level,
                        sink_json.value("hour", 0),
                        sink_json.value("minute", 0));
                }
            }
        }

        return initialize(config);
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "Failed to load config: %s\n", e.what());
        return false;
    }
}

// ControlLogger::~ControlLogger()
// {
//      if (initialized_) {
//         loggers_.clear();
//         spdlog::drop_all();
//         spdlog::shutdown();
//         // if(async_thread_pool_)
//         // {
//         //     async_thread_pool_.reset();
//         // }
//     }
// }
ControlLogger::~ControlLogger()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_)
        return;

    // 1. 清理本地引用
    loggers_.clear();

    // 2. 清理全局注册的logger
// #if SPDLOG_VERSION >= 0x00010900 // v1.9.0+
#if SPDLOG_VERSION >= 10900 // v1.9.0+
    spdlog::shutdown();          // 新版本只需这个
#else
    spdlog::drop_all();
    spdlog::shutdown(); // 旧版本双重保障
#endif

    // 3. 释放线程池（如果是异步模式）
    // async_thread_pool_.reset();
    if (async_thread_pool_)
    {
        async_thread_pool_.reset();
    }

    // 4. 标记为未初始化
    initialized_ = false;
}

std::shared_ptr<spdlog::logger> ControlLogger::getLogger(const std::string &logger_name)
{
    auto it = loggers_.find(logger_name);
    if (it != loggers_.end())
    {
        return it->second;
    }
    return nullptr;
}

// 异步模式
void ControlLogger::setupAsyncMode()
{
    async_thread_pool_ = std::make_shared<spdlog::details::thread_pool>(
        config_.async_queue_size, 1);
    for (auto &[name, logger] : loggers_)
    {
        auto new_logger = std::make_shared<spdlog::async_logger>(
            name, logger->sinks().begin(), logger->sinks().end(), async_thread_pool_,
            spdlog::async_overflow_policy::block);
        new_logger->set_level(logger->level());
        new_logger->flush_on(logger->flush_level());
        spdlog::drop(name);
        spdlog::register_logger(new_logger);
        loggers_[name] = new_logger;
    }
}

void ControlLogger::setupSyncMode()
{
    for (auto &[name, logger] : loggers_)
    {
        auto new_logger = std::make_shared<spdlog::logger>(
            name, logger->sinks().begin(), logger->sinks().end());
        new_logger->set_level(logger->level());
        new_logger->flush_on(logger->flush_level());
        spdlog::drop(name);
        spdlog::register_logger(new_logger);
        loggers_[name] = new_logger;
    }
}

bool ControlLogger::addConsoleSink(const std::string &logger_name, Level level)
{
    try
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_level(static_cast<spdlog::level::level_enum>(level));
        registerLogger(logger_name, {sink});
        return true;
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        fprintf(stderr, "Failed to add console sink: %s\n", ex.what());
        return false;
    }
}

bool ControlLogger::addRotatingFileSink(const std::string &logger_name,
                                        const std::string &filepath,
                                        Level level, size_t max_size,
                                        size_t max_files)
{
    try
    {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            filepath, max_size, max_files);
        sink->set_level(static_cast<spdlog::level::level_enum>(level));
        registerLogger(logger_name, {sink});
        return true;
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        fprintf(stderr, "Failed to add rotating file sink: %s\n", ex.what());
        return false;
    }
}

bool ControlLogger::addDailyFileSink(const std::string &logger_name,
                                     const std::string &filepath,
                                     Level level, int hour, int minute)
{
    try
    {
        auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            filepath, hour, minute);
        sink->set_level(static_cast<spdlog::level::level_enum>(level));
        registerLogger(logger_name, {sink});
        return true;
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        fprintf(stderr, "Failed to add daily file sink: %s\n", ex.what());
        return false;
    }
}

void ControlLogger::setLevel(const std::string &logger_name, Level level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto logger = getLogger(logger_name))
    {
        logger->set_level(static_cast<spdlog::level::level_enum>(level));
    }
}

void ControlLogger::setPattern(const std::string &pattern)
{
    std::lock_guard<std::mutex> lock(mutex_);
    spdlog::set_pattern(pattern);
}

void ControlLogger::toggleAsync(bool enable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_)
        return;

    if (enable && config_.mode != OutputMode::ASYNC)
    {
        config_.mode = OutputMode::ASYNC;
        setupAsyncMode();
    }
    else if (!enable && config_.mode == OutputMode::ASYNC)
    {
        config_.mode = OutputMode::SYNC;
        setupSyncMode();
    }
}

void ControlLogger::flushAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    // spdlog::flush_all();
    try
    {
        // 方法1：使用 apply_all（需要较新spdlog版本）
        spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger)
                          { logger->flush(); });

        // 或方法2：手动遍历（兼容性更好）
        // for (auto& [name, logger] : loggers_) {
        //     logger->flush();
        // }
    }
    catch (const std::exception &e)
    {
        // 错误处理逻辑
    }
}

void ControlLogger::registerLogger(const std::string &name,
                                   const std::vector<spdlog::sink_ptr> &sinks)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (auto existing = getLogger(name))
    {
        for (const auto &sink : sinks)
        {
            existing->sinks().push_back(sink);
        }
    }
    else
    {
        std::shared_ptr<spdlog::logger> logger;

        if (config_.mode == OutputMode::ASYNC)
        {
            logger = std::make_shared<spdlog::async_logger>(
                name, sinks.begin(), sinks.end(), async_thread_pool_,
                spdlog::async_overflow_policy::block);
        }
        else
        {
            logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        }

        logger->set_level(static_cast<spdlog::level::level_enum>(Level::TRACE));
        logger->flush_on(static_cast<spdlog::level::level_enum>(config_.flush_level));
        spdlog::register_logger(logger);
        loggers_[name] = logger;
    }
}