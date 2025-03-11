#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace cesium_server {

class Logger {
public:
    static void initialize(const std::string& log_name = "cesium_server",
                          const std::string& log_path = "logs") {
        try {
            // 创建控制台输出
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::debug);

            // 创建每日滚动文件输出
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                log_path + "/" + log_name + ".log", 0, 0);
            file_sink->set_level(spdlog::level::trace);

            // 创建多输出日志记录器
            std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
            auto logger = std::make_shared<spdlog::logger>(log_name, sinks.begin(), sinks.end());
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::debug);

            // 设置为默认日志记录器
            spdlog::set_default_logger(logger);

            // 设置日志格式
            spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [%t] %v");
            spdlog::info("日志系统初始化完成");
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "日志初始化失败: " << ex.what() << std::endl;
            throw;
        }
    }

    static void shutdown() {
        spdlog::shutdown();
    }
};

} // namespace cesium_server