#pragma once

#include "serial/serial_port.h"

#include <asio.hpp>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace remote_serial {

class SerialManager {
public:
    using DataCallback = std::function<void(const std::string& port, const std::vector<uint8_t>& data)>;

    explicit SerialManager(asio::io_context& io_context);
    ~SerialManager();

    bool OpenPort(const std::string& port,
                  int baudrate,
                  int databits,
                  int stopbits,
                  const std::string& parity);

    bool ClosePort(const std::string& port);

    bool WritePort(const std::string& port, const std::vector<uint8_t>& data);
    void WritePortAsync(const std::string& port, const std::vector<uint8_t>& data, std::function<void(bool)> callback);

    std::vector<std::string> ListPorts() const;

    void SetDataCallback(DataCallback cb);

private:
    struct PortEntry {
        std::shared_ptr<SerialPort> serial;
    };

    void OnSerialData(const std::string& port, const std::vector<uint8_t>& data);

    asio::io_context& io_context_;
    mutable std::mutex mutex_;
    std::map<std::string, PortEntry> ports_;
    DataCallback data_cb_;
};

} // namespace remote_serial
