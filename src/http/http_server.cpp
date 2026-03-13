#include "http/http_server.h"
#include "utils/logger.h"

namespace remote_serial {

HttpServer::HttpServer(SerialManager* manager)
    : manager_(manager) {
}

HttpServer::~HttpServer() {
    Stop();
}

bool HttpServer::Start(int port) {
    port_ = port;
    Logger::Info("HttpServer starting on port " + std::to_string(port));
    // TODO: initialize libhv HTTP + WebSocket server, register REST endpoints
    return true;
}

void HttpServer::Stop() {
    // TODO: stop libhv server
}

} // namespace remote_serial
