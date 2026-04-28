#pragma once

#include <asio.hpp>
#include <asio/serial_port.hpp>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace remote_serial {

class SerialPort : public std::enable_shared_from_this<SerialPort> {
public:
    using DataCallback = std::function<void(const std::vector<uint8_t>& data)>;

    explicit SerialPort(asio::io_context& io_context, std::string device);
    ~SerialPort();

    bool Open(int baudrate, int databits, int stopbits, const std::string& parity);
    void Close();

    bool Write(const std::vector<uint8_t>& data);
    void WriteAsync(const std::vector<uint8_t>& data, std::function<void(bool)> callback);

    // Async read: starts a chain of async_read_some calls.
    // callback is invoked on the io_context thread for each data chunk.
    // An empty vector signals an error or EOF.
    void StartAsyncRead(DataCallback callback);
    void StopAsyncRead();

    std::vector<uint8_t> Read();
    bool IsOpen() const;

private:
    void DoAsyncRead();

    asio::io_context& io_context_;
    std::string device_;
    std::unique_ptr<asio::serial_port> serial_port_;

    std::array<uint8_t, 1024> read_buffer_;
    std::atomic<bool> reading_{false};
    DataCallback data_callback_;
};

} // namespace remote_serial
