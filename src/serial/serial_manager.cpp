#include "serial/serial_manager.h"

#include <thread>

namespace remote_serial {

SerialManager::SerialManager(asio::io_context& io_context)
    : io_context_(io_context) {
}

SerialManager::~SerialManager() {
    // Ensure all ports are closed and threads are stopped.
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [port, entry] : ports_) {
        entry.running = false;
        if (entry.reader_thread.joinable()) {
            entry.reader_thread.join();
        }
        if (entry.serial) {
            entry.serial->Close();
        }
    }
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
        return true; // already open
    }

    PortEntry entry;
    entry.serial = std::make_shared<SerialPort>(io_context_, port);
    if (!entry.serial->Open(baudrate, databits, stopbits, parity)) {
        return false;
    }

    entry.running = true;
    StartReader(port, entry);

    ports_[port] = std::move(entry);
    return true;
}

bool SerialManager::ClosePort(const std::string& port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ports_.find(port);
    if (it == ports_.end()) {
        return false;
    }

    it->second.running = false;
    if (it->second.reader_thread.joinable()) {
        it->second.reader_thread.join();
    }
    if (it->second.serial) {
        it->second.serial->Close();
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
    // TODO: implement platform-specific serial enumeration.
    // For Linux, scan /dev/tty* or use udev
    // For Windows, use registry or SetupAPI
    return {"ttyUSB0", "ttyUSB1","/tmp/vserial"}; // placeholder
}

void SerialManager::StartReader(const std::string& port, PortEntry& entry) {
    // start a background thread to read data and invoke callback
    entry.reader_thread = std::thread([this, port, serial = entry.serial]() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = ports_.find(port);
                if (it == ports_.end() || !it->second.running) {
                    break;
                }
            }
            if (!serial || !serial->IsOpen()) {
                break;
            }
            auto data = serial->Read();
            if (!data.empty()) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (data_cb_) {
                    data_cb_(port, data);
                }
            }
            // small sleep to prevent busy loop if no data
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
}

} // namespace remote_serial
