#pragma once

#include "serial/serial_manager.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace remote_serial {

class ModbusManager {
public:
    struct ModbusResponse {
        int code = 0;
        std::string msg;
        std::vector<int> values;
        std::string raw_tx;
        std::string raw_rx;
    };

    explicit ModbusManager(SerialManager* manager);

    ModbusResponse ReadRegisters(const std::string& port, uint8_t slave, uint8_t function,
                                 uint16_t address, uint16_t quantity, int timeout_ms = 1000);

    ModbusResponse WriteRegisters(const std::string& port, uint8_t slave, uint8_t function,
                                  uint16_t address, uint16_t quantity_or_value,
                                  const std::vector<uint8_t>& values, int timeout_ms = 1000);

    void FeedResponse(const std::string& port, const std::vector<uint8_t>& data);

private:
    ModbusResponse WaitForResponse(int timeout_ms);
    ModbusResponse ParseResponse(uint8_t expected_function);

    static std::string ExceptionMessage(uint8_t exception_code);

    SerialManager* manager_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::string pending_port_;
    std::vector<uint8_t> rx_buffer_;
    bool waiting_ = false;
};

} // namespace remote_serial
