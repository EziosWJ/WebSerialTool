#include "http/http_server.h"
#include "serial/serial_manager.h"
#include "utils/logger.h"

#include <condition_variable>
#include <mutex>

int main(int argc, char** argv) {
    remote_serial::Logger::Init();

    remote_serial::SerialManager serial_manager;
    remote_serial::HttpServer server(&serial_manager);

    server.Start(8080);

    // TODO: graceful shutdown handling (signal, etc.)
    remote_serial::Logger::Info("Started web serial server on port 8080");

    // Block forever (placeholder)
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    std::condition_variable cv;
    cv.wait(lock);

    return 0;
}
