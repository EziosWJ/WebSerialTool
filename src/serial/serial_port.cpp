#include "serial/serial_port.h"

#include <asio/error.hpp>

namespace remote_serial {

SerialPort::SerialPort(asio::io_context& io_context, std::string device)
    : io_context_(io_context), device_(std::move(device)) {
}

SerialPort::~SerialPort() {
    Close();
}

bool SerialPort::Open(int baudrate, int databits, int stopbits, const std::string& parity) {
    try {
        serial_port_ = std::make_unique<asio::serial_port>(io_context_, device_);

        // Set baud rate
        serial_port_->set_option(asio::serial_port::baud_rate(baudrate));

        // Set character size
        serial_port_->set_option(asio::serial_port::character_size(databits));

        // Set stop bits
        if (stopbits == 1) {
            serial_port_->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
        } else if (stopbits == 2) {
            serial_port_->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::two));
        } else {
            return false;
        }

        // Set parity
        if (parity == "none") {
            serial_port_->set_option(asio::serial_port::parity(asio::serial_port::parity::none));
        } else if (parity == "odd") {
            serial_port_->set_option(asio::serial_port::parity(asio::serial_port::parity::odd));
        } else if (parity == "even") {
            serial_port_->set_option(asio::serial_port::parity(asio::serial_port::parity::even));
        } else {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

void SerialPort::Close() {
    if (serial_port_ && serial_port_->is_open()) {
        serial_port_->close();
    }
    serial_port_.reset();
}

bool SerialPort::Write(const std::vector<uint8_t>& data) {
    if (!serial_port_ || !serial_port_->is_open()) {
        return false;
    }

    try {
        asio::write(*serial_port_, asio::buffer(data));
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void SerialPort::WriteAsync(const std::vector<uint8_t>& data, std::function<void(bool)> callback) {
    if (!serial_port_ || !serial_port_->is_open()) {
        callback(false);
        return;
    }

    asio::async_write(*serial_port_, asio::buffer(data), [callback](const asio::error_code& ec, size_t) {
        callback(!ec);
    });
}

std::vector<uint8_t> SerialPort::Read() {
    // For simplicity, read available data synchronously
    // In real implementation, this should be asynchronous
    if (!serial_port_ || !serial_port_->is_open()) {
        return {};
    }

    std::vector<uint8_t> buffer(1024);
    try {
        size_t bytes_read = serial_port_->read_some(asio::buffer(buffer));
        buffer.resize(bytes_read);
        return buffer;
    } catch (const std::exception& e) {
        return {};
    }
}

bool SerialPort::IsOpen() const {
    return serial_port_ && serial_port_->is_open();
}

} // namespace remote_serial
