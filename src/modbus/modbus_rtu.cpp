#include "modbus/modbus_rtu.h"
#include <sstream>
#include <iomanip>

namespace remote_serial {

uint16_t ModbusRtu::Crc16(const std::vector<uint8_t>& data) {
    uint16_t crc = 0xFFFF; // CRC-16/Modbus 标准初始值和多项式
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

std::vector<uint8_t> ModbusRtu::BuildFrame(uint8_t slave, uint8_t function,
                                            uint16_t address, uint16_t quantity_or_value) {
    std::vector<uint8_t> frame = {
        slave,
        function,
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF),
        static_cast<uint8_t>((quantity_or_value >> 8) & 0xFF),
        static_cast<uint8_t>(quantity_or_value & 0xFF)
    };
    uint16_t crc = Crc16(frame);
    frame.push_back(crc & 0xFF);       // Modbus RTU: CRC 低字节在前
    frame.push_back((crc >> 8) & 0xFF);
    return frame;
}

std::vector<uint8_t> ModbusRtu::BuildWriteMultipleFrame(uint8_t slave, uint8_t function,
                                                        uint16_t address, uint16_t quantity,
                                                        const std::vector<uint8_t>& values) {
    std::vector<uint8_t> frame = {
        slave,
        function,
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF),
        static_cast<uint8_t>((quantity >> 8) & 0xFF),
        static_cast<uint8_t>(quantity & 0xFF),
        static_cast<uint8_t>(values.size())
    };
    frame.insert(frame.end(), values.begin(), values.end());
    uint16_t crc = Crc16(frame);
    frame.push_back(crc & 0xFF);
    frame.push_back((crc >> 8) & 0xFF);
    return frame;
}

bool ModbusRtu::VerifyCrc(const std::vector<uint8_t>& frame) {
    if (frame.size() < 3) return false;
    std::vector<uint8_t> payload(frame.begin(), frame.end() - 2);
    uint16_t computed = Crc16(payload);
    // 帧末尾 CRC 小端序：低字节在前
    uint16_t received = static_cast<uint16_t>(frame[frame.size() - 2]) |
                        (static_cast<uint16_t>(frame[frame.size() - 1]) << 8);
    return computed == received;
}

std::string ModbusRtu::ToHexString(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t b : data) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace remote_serial
