# Web 串口调试工具

**项目技术设计文档 v1.0**

---

# 1 项目概述

## 1.1 项目目标

开发一个 **跨平台 Web 串口调试工具**，用户在目标设备部署程序后，可以通过浏览器访问 Web 页面，对设备串口进行：

* 查看串口列表
* 配置串口参数
* 打开 / 关闭串口
* 发送串口数据
* 实时查看串口返回数据
* 保存串口通信日志

目标是实现 **跨平台、轻量、易部署的串口调试服务**。

支持运行环境：

* Linux
* Windows
* 嵌入式 Linux（Buildroot / OpenWRT / Yocto）

---

# 2 产品定位

## 2.1 典型使用场景

### 场景 1：嵌入式设备调试

设备部署该程序后：

```
http://设备IP:8080
```

工程师可远程调试设备串口。

---

### 场景 2：服务器串口调试

服务器连接多个串口设备：

```
USB转串口
RS485设备
工业传感器
PLC
```

通过 Web 页面统一管理。

---

### 场景 3：Windows串口调试

替代传统工具：

* SecureCRT
* 串口调试助手
* SSCOM

优势：

* Web UI
* 多用户访问
* 可远程访问

---

# 3 系统架构

系统采用 **三层架构**：

```
Browser
   │
   │ HTTP / WebSocket
   ▼
C++ Server (libhv)
   │
   │ Serial API
   ▼
Serial Device
```

架构示意：

```
+----------------------------+
|         Web Browser        |
|                            |
|  串口配置 UI               |
|  串口数据监控              |
|  串口发送控制              |
+-------------+--------------+
              │
              │ REST API
              │ WebSocket
              ▼
+----------------------------+
|        C++ Server          |
|                            |
|  HTTP Server (libhv)       |
|  WebSocket Server          |
|                            |
|  Serial Manager            |
|  SerialPort Driver         |
|                            |
+-------------+--------------+
              │
              │ 串口驱动
              ▼
+----------------------------+
|        Serial Device       |
|  /dev/ttyS*                |
|  /dev/ttyUSB*              |
|  COM*                      |
+----------------------------+
```

---

# 4 技术选型

## 4.1 后端语言

C++17

原因：

* 高性能
* 跨平台
* 嵌入式友好
* 依赖少

---

## 4.2 HTTP 框架

使用
**libhv**

原因：

* 轻量
* API 简单
* 支持 WebSocket
* 支持 HTTPS
* 支持 REST
* 跨平台

功能使用：

```
HTTP Server
WebSocket Server
JSON API
Static File Server
```

---

## 4.3 串口库

推荐：

### Boost.Asio

原因：

* 跨平台
* Linux + Windows
* 支持异步 IO
* 稳定成熟

支持：

```
boost::asio::serial_port
```

优点：

* 不依赖 POSIX
* Windows 无需额外封装

---

## 4.4 JSON

推荐：

```
nlohmann/json
```

原因：

* header-only
* 使用简单
* 兼容性好

---

## 4.5 日志库

推荐：

```
spdlog
```

功能：

* 文件日志
* 滚动日志
* 高性能

---

## 4.6 并发控制 & 数据格式

### 并发 / 访问冲突

* **串口写操作需要加锁**：多个客户端同时发送时，串口写必须串行化，避免交错数据。
* **读数据推送采用广播机制**：串口接收的帧在 `SerialManager` 内统一分发给所有订阅该串口的 WebSocket 客户端。
* **会话状态管理**：为每个 WebSocket 连接维护会话上下文（当前打开串口、权限等），并在串口关闭时释放资源。
* **串口断线/设备重连**：处理设备拔插、打开失败、读写异常等异常场景，并在客户端推送错误状态。

### 数据编码格式

串口数据通常是二进制流，Web UI 需要支持多种显示/发送格式：

* **HEX（16 进制）**：用于查看/发送二进制帧。
* **ASCII / UTF-8**：用于调试文本协议。
* **Base64**（可选）：用于传输任意二进制数据，避免转义问题。

在 API 设计中，建议在写入接口增加 `format` 字段：

```json
{
  "data": "48656c6c6f",
  "format": "hex"
}
```

或者：

```json
{
  "data": "hello",
  "format": "ascii"
}
```

接收数据时，WebSocket 可以提供一个默认格式（例如 `hex`），并允许客户端切换显示格式。

---

# 5 项目目录结构

推荐结构：

```
serial-web-tool
│
├── CMakeLists.txt
│
├── docs
│   ├── architecture.md
│   ├── api.md
│   └── build.md
│
├── third_party
│   ├── libhv
│   └── json
│
├── src
│
│   ├── main.cpp
│
│   ├── config
│   │   └── config_manager.cpp
│   │
│   ├── serial
│   │   ├── serial_port.cpp
│   │   ├── serial_port.h
│   │   ├── serial_manager.cpp
│   │   └── serial_manager.h
│   │
│   ├── http
│   │   ├── http_server.cpp
│   │   ├── http_server.h
│   │   ├── rest_api.cpp
│   │   └── websocket_server.cpp
│   │
│   ├── utils
│   │   ├── logger.cpp
│   │   └── utils.cpp
│
├── web
│   ├── index.html
│   ├── js
│   │   └── app.js
│   └── css
│
└── build
```

---

# 6 核心模块说明

## 6.1 SerialPort 模块

功能：

```
打开串口
关闭串口
读取数据
发送数据
设置串口参数
```

串口参数：

```
baudrate
databits
stopbits
parity
```

支持平台：

Linux

```
/dev/ttyS*
/dev/ttyUSB*
/dev/ttyACM*
```

Windows

```
COM1
COM2
COM3
```

---

## 6.2 SerialManager

作用：

统一管理多个串口。

支持：

```
多串口同时打开
串口状态管理
串口数据分发
```

数据流：

```
SerialPort → SerialManager → WebSocket
```

---

## 6.3 HTTP Server

使用 **libhv**

职责：

```
提供 REST API
提供 WebSocket
提供静态网页
```

示例：

```
GET  /api/ports
POST /api/port/open
POST /api/port/close
POST /api/port/write
```

---

## 6.4 WebSocket

用途：

**实时串口数据推送**

流程：

```
Serial Data
    │
    ▼
SerialManager
    │
    ▼
WebSocket Broadcast
    │
    ▼
Browser
```

优势：

* 实时
* 无轮询
* 延迟低

---

# 7 REST API 设计

## 获取串口列表

```
GET /api/ports
```

返回：

```json
{
  "ports":[
    "ttyUSB0",
    "ttyUSB1"
  ]
}
```

---

## 打开串口

```
POST /api/port/open
```

参数：

```json
{
  "port":"ttyUSB0",
  "baud":115200,
  "databits":8,
  "stopbits":1,
  "parity":"none"
}
```

---

## 发送数据

```
POST /api/port/write
```

参数：

```json
{
  "data":"48656c6c6f",
  "format":"hex"
}
```

* `format` 可选，支持：`hex` / `ascii` / `base64`（默认 `ascii`）。

---

# 8 Web UI 功能

Web 页面主要包含：

## 串口配置区

```
串口选择
波特率
数据位
停止位
校验位
打开串口
关闭串口
```

---

## 串口通信区

```
发送框
发送按钮
HEX发送
ASCII发送
```

---

## 数据监控区

实时显示：

```
串口接收数据
发送数据
时间戳
```

---

# 9 构建系统

推荐：

```
CMake
```

原因：

* 跨平台
* Linux / Windows 通用

---

## Linux 编译

```
mkdir build
cd build
cmake ..
make
```

生成：

```
serial_web_server
```

---

# 10 部署方式

## Linux

```
./serial_web_server
```

访问：

```
http://ip:8080
```

---

## 嵌入式设备

推荐：

```
Buildroot
OpenWRT
Yocto
```

方式：

```
静态编译
```

减少依赖。

---

# 11 安全设计（可选）

可选增加：

### 登录认证

```
用户名
密码
token
```

---

### HTTPS

libhv 支持：

```
TLS
```

---

# 12 项目优势

相比传统串口工具：

| 特性     | 本项目 |
| ------ | --- |
| 跨平台    | ✔   |
| Web UI | ✔   |
| 远程访问   | ✔   |
| 多设备支持  | ✔   |
| 嵌入式部署  | ✔   |

