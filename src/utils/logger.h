#pragma once

#include <string>

namespace remote_serial {

class Logger {
public:
    static void Init();
    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);
};

} // namespace remote_serial
