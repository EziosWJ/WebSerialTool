#include "serial/serial_manager.h"

namespace remote_serial {

SerialManager::SerialManager() = default;
SerialManager::~SerialManager() = default;

bool SerialManager::OpenPort(const std::string& port,
                             int baudrate,
                             int databits,
                             int stopbits,
                             const std::string& parity) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it != ports_.end() && it->second->IsOpen()) {
        return true; // already open
    }

    auto serial = std::make_unique<SerialPort>(port);
    if (!serial->Open(baudrate, databits, stopbits, parity)) {
        return false;
    }

    ports_[port] = std::move(serial);
    return true;
}

bool SerialManager::ClosePort(const std::string& port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end()) {
        return false;
    }
    it->second->Close();
    ports_.erase(it);
    return true;
}

bool SerialManager::WritePort(const std::string& port, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end() || !it->second->IsOpen()) {
        return false;
    }
    return it->second->Write(data);
}

std::vector<uint8_t> SerialManager::ReadPort(const std::string& port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end() || !it->second->IsOpen()) {
        return {};
    }
    return it->second->Read();
}

std::vector<std::string> SerialManager::ListPorts() const {
    // TODO: implement platform-specific serial enumeration.
    return {};
}

} // namespace remote_serial
