#include "http/http_server.h"
#include "modbus/modbus_rtu.h"
#include "utils/logger.h"
#include <functional>
#include <iostream>
#include <climits>
#include <unistd.h>
#include <sys/stat.h>
#include <hv/hv.h>
#include <hv/base64.h>

namespace remote_serial {

HttpServer::HttpServer(SerialManager* manager, const std::string& web_root)
    : manager_(manager), modbus_manager_(manager), web_root_(web_root) {
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

    // Resolve web root if not configured
    if (web_root_.empty()) {
        auto try_dir = [this](const std::string& dir) -> bool {
            std::string candidate = dir + "/web";
            struct stat st;
            if (stat(candidate.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                web_root_ = candidate;
                return true;
            }
            return false;
        };

        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len != -1) {
            buf[len] = '\0';
            std::string exe_path(buf);
            auto pos = exe_path.find_last_of('/');
            if (pos != std::string::npos) {
                std::string exe_dir = exe_path.substr(0, pos);
                // Check exe_dir/web first, then parent_dir/web
                if (!try_dir(exe_dir)) {
                    auto parent_pos = exe_dir.find_last_of('/');
                    if (parent_pos != std::string::npos) {
                        try_dir(exe_dir.substr(0, parent_pos));
                    }
                }
            }
        }
        if (web_root_.empty()) {
            web_root_ = "web";
        }
    }
    Logger::Info("Web root: " + web_root_);

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
    http_service_.GET("/api/logs", [this](const HttpContextPtr& ctx) { return HandleGetLogs(ctx); });
    http_service_.POST("/api/modbus/read", [this](const HttpContextPtr& ctx) { return HandleModbusRead(ctx); });
    http_service_.POST("/api/modbus/write", [this](const HttpContextPtr& ctx) { return HandleModbusWrite(ctx); });

    // Serve static files from web/ directory
    http_service_.Static("/", web_root_.c_str());
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
            size_t buf_len = (data_str.size() * 3) / 4;
            std::vector<uint8_t> buf(buf_len);
            int decoded = hv_base64_decode(data_str.data(), data_str.size(), buf.data());
            if (decoded < 0) {
                nlohmann::json response = {{"code", 1}, {"msg", "base64 decode failed"}};
                return ctx->sendJson(response);
            }
            buf.resize(decoded);
            data = std::move(buf);
        }

        bool success = manager_->WritePort(port, data);
        if (success) {
            log_saver_.OnTx(port, data);
        }
        nlohmann::json response = {{"code", success ? 0 : 1}, {"msg", success ? "ok" : "failed"}};
        return ctx->sendJson(response);
    } catch (const std::exception& e) {
        nlohmann::json response = {{"code", 1}, {"msg", "invalid request"}};
        return ctx->sendJson(response);
    }
}

int HttpServer::HandleGetLogs(const HttpContextPtr& ctx) {
    std::string port = ctx->param("port", "");
    nlohmann::json response;

    if (port.empty()) {
        // List available log files
        auto logs = log_saver_.ListLogs();
        response["logs"] = logs;
    } else {
        // Return log content for specific port
        response["port"] = port;
        response["content"] = log_saver_.ReadLog(port);
    }

    return ctx->sendJson(response);
}

void HttpServer::OnWebSocketOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req) {
    Logger::Info("WebSocket client connected");
    std::lock_guard<std::mutex> lock(ws_mutex_);
    ws_clients_.insert(channel);
    client_subscriptions_[channel] = {}; // initially subscribed to nothing
}

void HttpServer::OnWebSocketMessage(const WebSocketChannelPtr& channel, const std::string& msg) {
    try {
        auto json = nlohmann::json::parse(msg);
        std::string type = json["type"];

        std::lock_guard<std::mutex> lock(ws_mutex_);

        if (type == "subscribe") {
            std::string port = json["port"];
            client_subscriptions_[channel].insert(port);
            Logger::Info("Client subscribed to " + port);
        } else if (type == "unsubscribe") {
            std::string port = json["port"];
            client_subscriptions_[channel].erase(port);
            Logger::Info("Client unsubscribed from " + port);
        } else if (type == "list") {
            nlohmann::json resp;
            resp["type"] = "subscriptions";
            auto& ports = resp["ports"] = nlohmann::json::array();
            for (const auto& p : client_subscriptions_[channel]) {
                ports.push_back(p);
            }
            channel->send(resp.dump());
        }
    } catch (const std::exception& e) {
        Logger::Warn("Invalid WebSocket message: " + msg);
    }
}

void HttpServer::OnWebSocketClose(const WebSocketChannelPtr& channel) {
    Logger::Info("WebSocket client disconnected");
    std::lock_guard<std::mutex> lock(ws_mutex_);
    ws_clients_.erase(channel);
    client_subscriptions_.erase(channel);
}

int HttpServer::HandleModbusRead(const HttpContextPtr& ctx) {
    try {
        auto json = nlohmann::json::parse(ctx->body());
        std::string port = json["port"];
        uint8_t slave = json["slave"];
        uint8_t function = json["function"];
        uint16_t address = json["address"];
        uint16_t quantity = json["quantity"];
        int timeout = json.value("timeout", 1000);

        if (function < 1 || function > 4) {
            nlohmann::json response = {{"code", 1}, {"msg", "invalid function code, must be 1-4"}};
            return ctx->sendJson(response);
        }

        auto result = modbus_manager_.ReadRegisters(port, slave, function, address, quantity, timeout);

        nlohmann::json response;
        response["code"] = result.code;
        response["msg"] = result.msg;
        if (result.code == 0) {
            response["data"]["values"] = result.values;
            response["data"]["raw_tx"] = result.raw_tx;
            response["data"]["raw_rx"] = result.raw_rx;
        }
        return ctx->sendJson(response);
    } catch (const std::exception& e) {
        nlohmann::json response = {{"code", 1}, {"msg", std::string("invalid request: ") + e.what()}};
        return ctx->sendJson(response);
    }
}

int HttpServer::HandleModbusWrite(const HttpContextPtr& ctx) {
    try {
        auto json = nlohmann::json::parse(ctx->body());
        std::string port = json["port"];
        uint8_t slave = json["slave"];
        uint8_t function = json["function"];
        uint16_t address = json["address"];
        int timeout = json.value("timeout", 1000);

        uint16_t quantity_or_value = 0;
        std::vector<uint8_t> values;

        if (function == 5 || function == 6) {
            // Single write
            quantity_or_value = json["value"];
        } else if (function == 15 || function == 16) {
            // Multiple write
            quantity_or_value = json["quantity"];
            auto json_values = json["values"];
            if (function == 15) {
                // 前端发来的 0/1 数组按位打包，每字节 8 个线圈，LSB 对应第一个
                uint8_t byte_count = (quantity_or_value + 7) / 8;
                values.resize(byte_count, 0);
                for (size_t i = 0; i < json_values.size(); ++i) {
                    if (json_values[i].get<int>()) {
                        values[i / 8] |= (1 << (i % 8));
                    }
                }
            } else {
                // Registers: 16-bit values to bytes
                for (auto& v : json_values) {
                    uint16_t val = v.get<uint16_t>();
                    values.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
                    values.push_back(static_cast<uint8_t>(val & 0xFF));
                }
            }
        } else {
            nlohmann::json response = {{"code", 1}, {"msg", "invalid function code, must be 5/6/15/16"}};
            return ctx->sendJson(response);
        }

        auto result = modbus_manager_.WriteRegisters(port, slave, function, address,
                                                      quantity_or_value, values, timeout);

        nlohmann::json response;
        response["code"] = result.code;
        response["msg"] = result.msg;
        if (result.code == 0) {
            response["data"]["raw_tx"] = result.raw_tx;
            response["data"]["raw_rx"] = result.raw_rx;
        }
        return ctx->sendJson(response);
    } catch (const std::exception& e) {
        nlohmann::json response = {{"code", 1}, {"msg", std::string("invalid request: ") + e.what()}};
        return ctx->sendJson(response);
    }
}

void HttpServer::BroadcastSerialData(const std::string& port, const std::vector<uint8_t>& data) {
    // 必须先于 WebSocket 广播，确保 Modbus 响应被事务管理器优先消费
    modbus_manager_.FeedResponse(port, data);
    log_saver_.OnRx(port, data);

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
            auto it = client_subscriptions_.find(ch);
            if (it != client_subscriptions_.end() && (it->second.empty() || it->second.count(port))) {
                ch->send(payload);
            }
        }
    }
}

} // namespace remote_serial
