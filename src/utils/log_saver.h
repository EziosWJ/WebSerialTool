#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace remote_serial {

class LogSaver {
public:
    LogSaver();
    ~LogSaver();

    void OnTx(const std::string& port, const std::vector<uint8_t>& data);
    void OnRx(const std::string& port, const std::vector<uint8_t>& data);
    std::vector<std::string> ListLogs() const;
    std::string ReadLog(const std::string& port) const;

private:
    void WriteLog(const std::string& port, char dir, const std::vector<uint8_t>& data);
    std::string LogFilePath(const std::string& port) const;

    mutable std::mutex mutex_;
    std::string log_dir_;
};

} // namespace remote_serial
