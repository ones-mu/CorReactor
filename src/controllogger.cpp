#include "controllogger.h"
#include <ifstream>

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

        //解析日志级别
        auto parse_level = [](const std::string &level_str) -> Level {
            static const std::unordered_map<std::string, Level> level_map = {
                {"TRACE", Level::TRACE},
                {"DEBUG", Level::DEBUG},
                {"INFO", Level::INFO},
                {"WARN", Level::WARN},
                {"ERROR", Level::ERROR},
                {"CRITICAL", Level::CRITICAL},
                {"OFF", Level::OFF}
            };
            auto it = level_map.find(level_str);
            return it != level_map.end() ? it->second : Level::INFO;
        };

        config.flush_level = parse_level(config_json.value("flush_level", "WARN"));

        // 解析日志器和sinks
        for (const auto& logger_json : config_json["loggers"]) {
            std::string logger_name = logger_json["name"];
            
            for (const auto& sink_json : logger_json["sinks"]) {
                std::string type = sink_json["type"];
                Level level = parse_level(sink_json["level"]);
                
                if (type == "console") {
                    addConsoleSink(logger_name, level);
                } else if (type == "rotating_file") {
                    addRotatingFileSink(
                        logger_name,
                        sink_json["filepath"],
                        level,
                        sink_json.value("max_size", 1024 * 1024 * 5),
                        sink_json.value("max_files", 3)
                    );
                } else if (type == "daily_file") {
                    addDailyFileSink(
                        logger_name,
                        sink_json["filepath"],
                        level,
                        sink_json.value("hour", 0),
                        sink_json.value("minute", 0)
                    );
                }
            }
        }

        return initialize(config);
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to load config: %s\n", e.what());
        return false;
    }
}

ControlLogger::~ControlLogger()
{
    if(initialized_)
    {
        spdlog::drop_all();
        spdlog::shutdown();
    }
}

std::shared_ptr<spdlog::logger> ControlLogger::getLogger(const std::string &logger_name)
{
    auto it=loggers_.find(logger_name);
    if(it!=loggers_.end())
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
