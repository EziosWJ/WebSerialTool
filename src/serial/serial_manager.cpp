#include "serial/serial_manager.h"
#include "utils/logger.h"
#include <glob.h>
#include <sys/stat.h>

namespace remote_serial {

SerialManager::SerialManager(asio::io_context& io_context)
    : io_context_(io_context) {
}

SerialManager::~SerialManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    ports_.clear();
}

void SerialManager::SetDataCallback(DataCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_cb_ = std::move(cb);
}

bool SerialManager::OpenPort(const std::string& port,
                             int baudrate,
                             int databits,
                             int stopbits,
                             const std::string& parity) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it != ports_.end() && it->second.serial && it->second.serial->IsOpen()) {
        return true;
    }

    PortEntry entry;
    entry.serial = std::make_shared<SerialPort>(io_context_, port);
    if (!entry.serial->Open(baudrate, databits, stopbits, parity)) {
        return false;
    }

    // Start async reads - the callback wraps data_cb_ with the port name
    entry.serial->StartAsyncRead([this, port](const std::vector<uint8_t>& data) {
        if (data.empty()) {
            // error/EOF: port disconnected, remove it
            Logger::Warn("Serial port " + port + " disconnected");
            ClosePort(port);
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_cb_) {
            data_cb_(port, data);
        }
    });

    ports_[port] = std::move(entry);
    return true;
}

bool SerialManager::ClosePort(const std::string& port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end()) {
        return false;
    }

    ports_.erase(it);
    return true;
}

bool SerialManager::WritePort(const std::string& port, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end() || !it->second.serial || !it->second.serial->IsOpen()) {
        return false;
    }
    return it->second.serial->Write(data);
}

void SerialManager::WritePortAsync(const std::string& port, const std::vector<uint8_t>& data, std::function<void(bool)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end() || !it->second.serial || !it->second.serial->IsOpen()) {
        callback(false);
        return;
    }
    it->second.serial->WriteAsync(data, callback);
}

std::vector<std::string> SerialManager::ListPorts() const {
    std::vector<std::string> ports;

    const char* patterns[] = {
        "/dev/ttyS[0-9]*",
        "/dev/ttyUSB[0-9]*",
        "/dev/ttyACM[0-9]*"
    };

    for (const auto& pattern : patterns) {
        glob_t glob_result;
        int glob_ret = glob(pattern, GLOB_NOSORT, nullptr, &glob_result);
        if (glob_ret == 0) {
            for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
                ports.emplace_back(glob_result.gl_pathv[i]);
            }
        }
        globfree(&glob_result);
    }

    const char* vserial = "/tmp/vserial";
    struct stat buffer;
    if (stat(vserial, &buffer) == 0) {
        ports.emplace_back(vserial);
    }

    return ports;
}

} // namespace remote_serial
