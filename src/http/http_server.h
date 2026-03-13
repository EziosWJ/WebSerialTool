#pragma once

#include "serial/serial_manager.h"

#include <hv/HttpService.h>
#include <hv/WebSocketServer.h>
#include <nlohmann/json.hpp>
#include <set>
#include <mutex>
#include <string>

namespace remote_serial {

class HttpServer {
public:
    explicit HttpServer(SerialManager* manager);
    ~HttpServer();

    bool Start(int port);
    void Stop();

private:
    void RegisterRoutes();
    void RegisterWebSocket();

    // REST API handlers
    int HandleGetPorts(const HttpContextPtr& ctx);
    int HandleOpenPort(const HttpContextPtr& ctx);
    int HandleClosePort(const HttpContextPtr& ctx);
    int HandleWritePort(const HttpContextPtr& ctx);

    // WebSocket handlers
    void OnWebSocketOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req);
    void OnWebSocketMessage(const WebSocketChannelPtr& channel, const std::string& msg);
    void OnWebSocketClose(const WebSocketChannelPtr& channel);

    void BroadcastSerialData(const std::string& port, const std::vector<uint8_t>& data);

    SerialManager* manager_ = nullptr;
    hv::HttpService http_service_;
    hv::WebSocketService ws_service_;
    hv::WebSocketServer server_;
    int port_ = 0;

    // WebSocket clients
    std::mutex ws_mutex_;
    std::set<WebSocketChannelPtr> ws_clients_;
};

} // namespace remote_serial
