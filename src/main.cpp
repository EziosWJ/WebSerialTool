#include "http/http_server.h"
#include "serial/serial_manager.h"
#include "utils/logger.h"

#include <asio.hpp>
#include <csignal>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

// Self-pipe trick for signal handling (async-signal-safe)
static int g_signal_fds[2];

extern "C" {
    static void signal_handler(int) {
        char c = 1;
        write(g_signal_fds[1], &c, 1);
    }
}

int main(int argc, char** argv) {
    remote_serial::Logger::Init();

    // Parse command-line arguments
    std::string web_root;
    int port = 8080;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--web-root" && i + 1 < argc) {
            web_root = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        }
    }

    // Setup self-pipe for signal notification
    pipe(g_signal_fds);
    fcntl(g_signal_fds[1], F_SETFL, O_NONBLOCK);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    asio::io_context io_context;
    remote_serial::SerialManager serial_manager(io_context);
    remote_serial::HttpServer server(&serial_manager, web_root);

    // Start io_context in a separate thread
    std::thread io_thread([&io_context]() {
        io_context.run();
    });

    server.Start(port);
    remote_serial::Logger::Info("Started web serial server on port 8080");

    // Block until signal received
    char c;
    read(g_signal_fds[0], &c, 1);
    remote_serial::Logger::Info("Shutting down...");

    // Cleanup
    server.Stop();
    io_context.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }

    close(g_signal_fds[0]);
    close(g_signal_fds[1]);

    return 0;
}
