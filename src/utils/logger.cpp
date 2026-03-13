#include "utils/logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace remote_serial {

void Logger::Init() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("remote_serial", console_sink);
    logger->set_level(spdlog::level::info);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
    spdlog::set_default_logger(logger);
}

void Logger::Info(const std::string& msg) {
    spdlog::info(msg);
}

void Logger::Warn(const std::string& msg) {
    spdlog::warn(msg);
}

void Logger::Error(const std::string& msg) {
    spdlog::error(msg);
}

} // namespace remote_serial
