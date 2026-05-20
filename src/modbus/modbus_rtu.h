#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace remote_serial {

class ModbusRtu {
public:
    ModbusRtu() = delete;

    static uint16_t Crc16(const std::vector<uint8_t>& data);

    static std::vector<uint8_t> BuildFrame(uint8_t slave, uint8_t function,
                                           uint16_t address, uint16_t quantity_or_value);

    static std::vector<uint8_t> BuildWriteMultipleFrame(uint8_t slave, uint8_t function,
                                                        uint16_t address, uint16_t quantity,
                                                        const std::vector<uint8_t>& values);

    static bool VerifyCrc(const std::vector<uint8_t>& frame);

    static std::string ToHexString(const std::vector<uint8_t>& data);
};

} // namespace remote_serial
