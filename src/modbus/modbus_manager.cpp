#include "modbus/modbus_manager.h"
#include "modbus/modbus_rtu.h"
#include "utils/logger.h"

namespace remote_serial {

ModbusManager::ModbusManager(SerialManager* manager)
    : manager_(manager) {
}

ModbusManager::ModbusResponse ModbusManager::ReadRegisters(
        const std::string& port, uint8_t slave, uint8_t function,
        uint16_t address, uint16_t quantity, int timeout_ms) {
    auto frame = ModbusRtu::BuildFrame(slave, function, address, quantity);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_port_ = port;
        rx_buffer_.clear();
        waiting_ = true;
    }

    if (!manager_->WritePort(port, frame)) {
        return {1, "serial write failed", {}, ModbusRtu::ToHexString(frame), ""};
    }

    auto resp = WaitForResponse(timeout_ms);
    if (resp.code != 0) {
        resp.raw_tx = ModbusRtu::ToHexString(frame);
        return resp;
    }

    resp.raw_tx = ModbusRtu::ToHexString(frame);
    resp.raw_rx = ModbusRtu::ToHexString(rx_buffer_);

    // Parse response
    auto parsed = ParseResponse(function);
    parsed.raw_tx = resp.raw_tx;
    parsed.raw_rx = resp.raw_rx;
    return parsed;
}

ModbusManager::ModbusResponse ModbusManager::WriteRegisters(
        const std::string& port, uint8_t slave, uint8_t function,
        uint16_t address, uint16_t quantity_or_value,
        const std::vector<uint8_t>& values, int timeout_ms) {
    std::vector<uint8_t> frame;
    if (function == 15 || function == 16) {
        frame = ModbusRtu::BuildWriteMultipleFrame(slave, function, address, quantity_or_value, values);
    } else {
        frame = ModbusRtu::BuildFrame(slave, function, address, quantity_or_value);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_port_ = port;
        rx_buffer_.clear();
        waiting_ = true;
    }

    if (!manager_->WritePort(port, frame)) {
        return {1, "serial write failed", {}, ModbusRtu::ToHexString(frame), ""};
    }

    auto resp = WaitForResponse(timeout_ms);
    if (resp.code != 0) {
        resp.raw_tx = ModbusRtu::ToHexString(frame);
        return resp;
    }

    resp.raw_tx = ModbusRtu::ToHexString(frame);
    resp.raw_rx = ModbusRtu::ToHexString(rx_buffer_);

    auto parsed = ParseResponse(function);
    parsed.raw_tx = resp.raw_tx;
    parsed.raw_rx = resp.raw_rx;
    return parsed;
}

// SerialManager 只有一个 DataCallback（已用于广播），由 BroadcastSerialData 主动调用避免回调冲突
void ModbusManager::FeedResponse(const std::string& port, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!waiting_ || port != pending_port_) {
        return;
    }

    rx_buffer_.insert(rx_buffer_.end(), data.begin(), data.end());

    // 串口数据分块到达，需根据功能码判断是否收完整帧
    bool complete = false;
    if (rx_buffer_.size() >= 3) {
        uint8_t func = rx_buffer_[1];
        if (func & 0x80) {
            complete = rx_buffer_.size() >= 5;                          // 异常响应固定 5 字节
        } else if (func >= 1 && func <= 4) {
            uint8_t byte_count = rx_buffer_[2];
            complete = rx_buffer_.size() >= static_cast<size_t>(3 + byte_count + 2); // 长度由 byte_count 字段决定
        } else if (func == 5 || func == 6 || func == 15 || func == 16) {
            complete = rx_buffer_.size() >= 8;                          // 写响应固定 8 字节
        }
    }

    if (complete) {
        waiting_ = false;
        cv_.notify_one();
    }
}

ModbusManager::ModbusResponse ModbusManager::WaitForResponse(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    // FeedResponse 收到完整帧后会把 waiting_ 设为 false
    bool got_response = cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                     [this] { return !waiting_; });
    if (!got_response) {
        waiting_ = false;
        return {1, "modbus response timeout", {}, "", ""};
    }
    return {0, "ok", {}, "", ""};
}

ModbusManager::ModbusResponse ModbusManager::ParseResponse(uint8_t expected_function) {
    if (rx_buffer_.size() < 3) {
        return {1, "response too short", {}, "", ""};
    }

    if (!ModbusRtu::VerifyCrc(rx_buffer_)) {
        return {1, "CRC check failed", {}, "", ModbusRtu::ToHexString(rx_buffer_)};
    }

    uint8_t func = rx_buffer_[1];

    // Exception response
    if (func & 0x80) {
        uint8_t exception_code = rx_buffer_[2];
        return {1, "modbus exception: " + ExceptionMessage(exception_code), {}, "", ModbusRtu::ToHexString(rx_buffer_)};
    }

    if (func != expected_function) {
        return {1, "unexpected function code", {}, "", ModbusRtu::ToHexString(rx_buffer_)};
    }

    // Parse read response
    if (func >= 1 && func <= 4) {
        uint8_t byte_count = rx_buffer_[2];
        std::vector<int> values;

        if (func == 1 || func == 2) {
            // Modbus 线圈/离散输入按位打包，每字节 8 个值，LSB 对应第一个
            for (size_t i = 0; i < byte_count; ++i) {
                uint8_t byte = rx_buffer_[3 + i];
                for (int bit = 0; bit < 8; ++bit) {
                    values.push_back((byte >> bit) & 0x01);
                }
            }
        } else {
            // Holding/Input Registers: 16-bit values
            for (size_t i = 0; i + 1 < byte_count; i += 2) {
                uint16_t val = (static_cast<uint16_t>(rx_buffer_[3 + i]) << 8) |
                               static_cast<uint16_t>(rx_buffer_[3 + i + 1]);
                values.push_back(val);
            }
        }

        return {0, "ok", values, "", ModbusRtu::ToHexString(rx_buffer_)};
    }

    // Write response: no data values to extract
    return {0, "ok", {}, "", ModbusRtu::ToHexString(rx_buffer_)};
}

std::string ModbusManager::ExceptionMessage(uint8_t code) {
    switch (code) {
        case 0x01: return "illegal function (非法功能码)";
        case 0x02: return "illegal data address (非法数据地址)";
        case 0x03: return "illegal data value (非法数据值)";
        case 0x04: return "slave device failure (从站设备故障)";
        case 0x05: return "acknowledge (确认)";
        case 0x06: return "slave device busy (从站设备忙)";
        default:   return "unknown (" + std::to_string(code) + ")";
    }
}

} // namespace remote_serial
