#include "http/http_server.h"
#include "serial/serial_manager.h"
#include "utils/logger.h"

#include <asio.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>

int main(int argc, char** argv) {
    remote_serial::Logger::Init();

    asio::io_context io_context;
    remote_serial::SerialManager serial_manager(io_context);
    remote_serial::HttpServer server(&serial_manager);

    // Start io_context in a separate thread
    std::thread io_thread([&io_context]() {
        io_context.run();
    });

    server.Start(8080);

    // TODO: graceful shutdown handling (signal, etc.)
    remote_serial::Logger::Info("Started web serial server on port 8080");

    // Block forever (placeholder)
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    std::condition_variable cv;
    cv.wait(lock);

    // Cleanup
    io_context.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }

    return 0;
}
