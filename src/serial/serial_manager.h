#pragma once

#include "serial/serial_port.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace remote_serial {

class SerialManager {
public:
    SerialManager();
    ~SerialManager();

    bool OpenPort(const std::string& port,
                  int baudrate,
                  int databits,
                  int stopbits,
                  const std::string& parity);

    bool ClosePort(const std::string& port);

    bool WritePort(const std::string& port, const std::vector<uint8_t>& data);

    std::vector<uint8_t> ReadPort(const std::string& port);

    std::vector<std::string> ListPorts() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<SerialPort>> ports_;
};

} // namespace remote_serial
