#include "utils/logger.h"

#include <iostream>

namespace remote_serial {

void Logger::Init() {
    // TODO: Initialize spdlog or other logging backend.
}

void Logger::Info(const std::string& msg) {
    std::cout << "[INFO] " << msg << "\n";
}

void Logger::Warn(const std::string& msg) {
    std::cout << "[WARN] " << msg << "\n";
}

void Logger::Error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << "\n";
}

} // namespace remote_serial
