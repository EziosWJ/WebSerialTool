#include "http/http_server.h"
#include "utils/logger.h"
#include <functional>
#include <iostream>
#include <hv/hv.h>

namespace remote_serial {

HttpServer::HttpServer(SerialManager* manager)
    : manager_(manager) {
    if (manager_) {
        manager_->SetDataCallback([this](const std::string& port, const std::vector<uint8_t>& data) {
            BroadcastSerialData(port, data);
        });
    }
}

HttpServer::~HttpServer() {
    Stop();
}

bool HttpServer::Start(int port) {
    port_ = port;
    Logger::Info("HttpServer starting on port " + std::to_string(port));

    RegisterRoutes();
    RegisterWebSocket();

    server_.registerHttpService(&http_service_);
    server_.registerWebSocketService(&ws_service_);
    server_.port = port;

    if (server_.start() != 0) {
        Logger::Error("Failed to start server");
        return false;
    }

    Logger::Info("Server started on port " + std::to_string(port));
    return true;
}

void HttpServer::Stop() {
    server_.stop();
}

void HttpServer::RegisterRoutes() {
    http_service_.GET("/api/ports", [this](const HttpContextPtr& ctx) { return HandleGetPorts(ctx); });
    http_service_.POST("/api/port/open", [this](const HttpContextPtr& ctx) { return HandleOpenPort(ctx); });
    http_service_.POST("/api/port/close", [this](const HttpContextPtr& ctx) { return HandleClosePort(ctx); });
    http_service_.POST("/api/port/write", [this](const HttpContextPtr& ctx) { return HandleWritePort(ctx); });

    // Serve static files from web/ directory
    http_service_.Static("/", "/home/wangjian/cpp/remoteSerial/web");
}

void HttpServer::RegisterWebSocket() {
    ws_service_.onopen = [this](const WebSocketChannelPtr& channel, const HttpRequestPtr& req) {
        OnWebSocketOpen(channel, req);
    };
    ws_service_.onmessage = [this](const WebSocketChannelPtr& channel, const std::string& msg) {
        OnWebSocketMessage(channel, msg);
    };
    ws_service_.onclose = [this](const WebSocketChannelPtr& channel) {
        OnWebSocketClose(channel);
    };
}

int HttpServer::HandleGetPorts(const HttpContextPtr& ctx) {
    auto ports = manager_->ListPorts();
    nlohmann::json response = {{"ports", ports}};
    return ctx->sendJson(response);
}

int HttpServer::HandleOpenPort(const HttpContextPtr& ctx) {
    try {
        auto json = nlohmann::json::parse(ctx->body());
        std::string port = json["port"];
        int baud = json["baud"];
        int databits = json["databits"];
        int stopbits = json["stopbits"];
        std::string parity = json["parity"];

        bool success = manager_->OpenPort(port, baud, databits, stopbits, parity);
        nlohmann::json response = {{"code", success ? 0 : 1}, {"msg", success ? "ok" : "failed"}};
        return ctx->sendJson(response);
    } catch (const std::exception& e) {
        nlohmann::json response = {{"code", 1}, {"msg", "invalid request"}};
        return ctx->sendJson(response);
    }
}

int HttpServer::HandleClosePort(const HttpContextPtr& ctx) {
    try {
        auto json = nlohmann::json::parse(ctx->body());
        std::string port = json["port"];

        bool success = manager_->ClosePort(port);
        nlohmann::json response = {{"code", success ? 0 : 1}, {"msg", success ? "ok" : "failed"}};
        return ctx->sendJson(response);
    } catch (const std::exception& e) {
        nlohmann::json response = {{"code", 1}, {"msg", "invalid request"}};
        return ctx->sendJson(response);
    }
}

int HttpServer::HandleWritePort(const HttpContextPtr& ctx) {
    try {
        auto json = nlohmann::json::parse(ctx->body());
        std::string port = json["port"];
        std::string data_str = json["data"];
        std::string format = json.value("format", "ascii");

        std::vector<uint8_t> data;
        if (format == "hex") {
            // Convert hex string to bytes
            for (size_t i = 0; i < data_str.length(); i += 2) {
                std::string byte_str = data_str.substr(i, 2);
                data.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
            }
        } else if (format == "ascii") {
            data.assign(data_str.begin(), data_str.end());
        } else if (format == "base64") {
            // TODO: implement base64 decode
            data.assign(data_str.begin(), data_str.end());
        }

        bool success = manager_->WritePort(port, data);
        nlohmann::json response = {{"code", success ? 0 : 1}, {"msg", success ? "ok" : "failed"}};
        return ctx->sendJson(response);
    } catch (const std::exception& e) {
        nlohmann::json response = {{"code", 1}, {"msg", "invalid request"}};
        return ctx->sendJson(response);
    }
}

void HttpServer::OnWebSocketOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req) {
    Logger::Info("WebSocket client connected");
    std::lock_guard<std::mutex> lock(ws_mutex_);
    ws_clients_.insert(channel);
}

void HttpServer::OnWebSocketMessage(const WebSocketChannelPtr& channel, const std::string& msg) {
    // TODO: handle WebSocket messages, e.g., subscribe to port data
    Logger::Info("WebSocket message: " + msg);
}

void HttpServer::OnWebSocketClose(const WebSocketChannelPtr& channel) {
    Logger::Info("WebSocket client disconnected");
    std::lock_guard<std::mutex> lock(ws_mutex_);
    ws_clients_.erase(channel);
}

void HttpServer::BroadcastSerialData(const std::string& port, const std::vector<uint8_t>& data) {
    nlohmann::json message;
    message["port"] = port;
    message["data"] = "";

    // encode to hex
    static const char* HEX = "0123456789ABCDEF";
    std::string hex;
    hex.reserve(data.size() * 2);
    for (uint8_t b : data) {
        hex.push_back(HEX[(b >> 4) & 0xF]);
        hex.push_back(HEX[b & 0xF]);
    }
    message["hex"] = hex;

    // encode to ascii (printable chars, others as '.')
    std::string ascii;
    ascii.reserve(data.size());
    for (uint8_t b : data) {
        if (b >= 32 && b < 127) {
            ascii += static_cast<char>(b);
        } else {
            ascii += '.';
        }
    }
    message["ascii"] = ascii;

    std::string payload = message.dump();

    std::lock_guard<std::mutex> lock(ws_mutex_);
    for (auto& ch : ws_clients_) {
        if (ch && ch->isConnected()) {
            ch->send(payload);
        }
    }
}

} // namespace remote_serial
