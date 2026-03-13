#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace remote_serial {

class SerialPort {
public:
    explicit SerialPort(std::string device);
    ~SerialPort();

    bool Open(int baudrate, int databits, int stopbits, const std::string& parity);
    void Close();

    bool Write(const std::vector<uint8_t>& data);
    std::vector<uint8_t> Read();

    bool IsOpen() const;

private:
    std::string device_;
    bool opened_ = false;
};

} // namespace remote_serial
