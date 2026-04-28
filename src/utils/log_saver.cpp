#include "utils/log_saver.h"
#include "utils/logger.h"

#include <chrono>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace remote_serial {

LogSaver::LogSaver() {
    log_dir_ = "logs";
    mkdir(log_dir_.c_str(), 0755);
}

LogSaver::~LogSaver() = default;

std::string LogSaver::LogFilePath(const std::string& port) const {
    // Sanitize port name for filesystem (replace / with _)
    std::string safe_name = port;
    for (auto& c : safe_name) {
        if (c == '/') c = '_';
    }
    return log_dir_ + "/" + safe_name + ".log";
}

void LogSaver::WriteLog(const std::string& port, char dir, const std::vector<uint8_t>& data) {
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::tm tm;
    localtime_r(&now_t, &tm);

    // Build hex string
    std::ostringstream hex;
    hex << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < data.size(); ++i) {
        hex << std::setw(2) << static_cast<int>(data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < data.size()) {
            hex << "\n    ";
        } else if (i + 1 < data.size()) {
            hex << " ";
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(LogFilePath(port), std::ios::app);
    if (!file) return;

    file << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "."
         << std::setw(3) << std::setfill('0') << ms.count() << "]"
         << " [" << dir << "X] " << hex.str() << "\n";
}

void LogSaver::OnTx(const std::string& port, const std::vector<uint8_t>& data) {
    WriteLog(port, 'T', data);
}

void LogSaver::OnRx(const std::string& port, const std::vector<uint8_t>& data) {
    WriteLog(port, 'R', data);
}

std::vector<std::string> LogSaver::ListLogs() const {
    std::vector<std::string> logs;
    DIR* dir = opendir(log_dir_.c_str());
    if (!dir) return logs;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".log") {
            // Convert back: _dev_ttyUSB0.log -> /dev/ttyUSB0
            std::string port_name = name.substr(0, name.size() - 4);
            for (auto& c : port_name) {
                if (c == '_') c = '/';
            }
            logs.push_back(port_name);
        }
    }
    closedir(dir);
    return logs;
}

std::string LogSaver::ReadLog(const std::string& port) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream file(LogFilePath(port));
    if (!file) return "";
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

} // namespace remote_serial
