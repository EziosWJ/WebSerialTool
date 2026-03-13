#pragma once

#include "serial/serial_manager.h"

#include <string>

namespace remote_serial {

class HttpServer {
public:
    explicit HttpServer(SerialManager* manager);
    ~HttpServer();

    bool Start(int port);
    void Stop();

private:
    SerialManager* manager_ = nullptr;
    int port_ = 0;
};

} // namespace remote_serial
