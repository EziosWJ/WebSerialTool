#pragma once

#include <asio.hpp>
#include <asio/serial_port.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace remote_serial {

class SerialPort {
public:
    explicit SerialPort(asio::io_context& io_context, std::string device);
    ~SerialPort();

    bool Open(int baudrate, int databits, int stopbits, const std::string& parity);
    void Close();

    bool Write(const std::vector<uint8_t>& data);
    void WriteAsync(const std::vector<uint8_t>& data, std::function<void(bool)> callback);
    std::vector<uint8_t> Read();

    bool IsOpen() const;

private:
    asio::io_context& io_context_;
    std::string device_;
    std::unique_ptr<asio::serial_port> serial_port_;
};

} // namespace remote_serial
