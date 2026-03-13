#include "serial/serial_port.h"

namespace remote_serial {

SerialPort::SerialPort(std::string device)
    : device_(std::move(device)) {
}

SerialPort::~SerialPort() {
    Close();
}

bool SerialPort::Open(int /*baudrate*/, int /*databits*/, int /*stopbits*/, const std::string& /*parity*/) {
    // TODO: implement platform-specific serial open (Boost.Asio, termios, Win32)
    opened_ = true;
    return opened_;
}

void SerialPort::Close() {
    if (!opened_) return;
    // TODO: close serial port
    opened_ = false;
}

bool SerialPort::Write(const std::vector<uint8_t>& /*data*/) {
    // TODO: implement write to serial port
    return false;
}

std::vector<uint8_t> SerialPort::Read() {
    // TODO: implement read from serial port
    return {};
}

bool SerialPort::IsOpen() const {
    return opened_;
}

} // namespace remote_serial
