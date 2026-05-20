# WebSerialTool

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20Embedded-lightgrey.svg)]()

[English](README.md) | [中文](README_CN.md)

> 轻量级跨平台 Web 串口调试工具，支持 Modbus RTU 协议调试。
> 通过浏览器远程访问和控制串口设备，无需安装桌面软件。

---

## 功能特性

- **Web 界面** — 工业控制台风格，任意现代浏览器即可调试串口
- **远程访问** — 通过网络连接设备（如 `http://192.168.1.10:8080`）
- **跨平台** — Linux、Windows、嵌入式 Linux（Buildroot / OpenWRT / Yocto）
- **完整串口配置** — 波特率、数据位、停止位、校验位
- **多种数据格式** — 支持 ASCII、HEX、Base64 发送/接收
- **实时数据推送** — WebSocket 推送，无轮询延迟
- **Modbus RTU** — 专用协议面板，支持全部 8 个功能码（0x01-0x10），响应表格化展示，自动轮询
- **日志保存** — 导出通信日志用于分析
- **零运行时依赖** — 可静态链接

---

## 界面预览

```
┌─────────────────────────────────────────────────────┐
│  △ SERIAL          [SERIAL] [MODBUS]                │
│    DEBUG TOOL       状态: ONLINE                     │
├─────────────────────────────────────────────────────┤
│  端口             │  控制台                           │
│  [/dev/ttyUSB0▾]  │  10:23:45.123 RX /dev/ttyUSB0 ..│
│  参数             │  10:23:45.456 TX /dev/ttyUSB0 ..│
│  波特率  115200   │  10:23:46.789 RX /dev/ttyUSB0 ..│
│  数据位  8        │                                  │
│  停止位  1        │                                  │
│  校验    NONE     │                                  │
│  [打开]  [关闭]   │                                  │
└─────────────────────────────────────────────────────┘
```

---

## 快速开始

### 环境要求

- CMake >= 3.14
- C++17 编译器（GCC >= 7 或 MSVC >= 2019）
- Git
- Ninja（推荐）

### 构建

```bash
git clone https://github.com/EziosWJ/WebSerialTool.git
cd WebSerialTool

# 使用 CMake Presets（推荐）
cmake --preset x64
cmake --build out/build/x64

# 或传统方式
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 运行

```bash
./out/build/x64/remoteSerial                     # 默认端口 8080
./out/build/x64/remoteSerial --port 9090          # 指定端口
./out/build/x64/remoteSerial --web-root /path/to/web
```

打开浏览器访问 `http://localhost:8080`。

> Web 根目录自动检测优先级：`--web-root` 参数 > exe 同级 `web/` > exe 上级 `web/` > 回退 CWD 下 `web/`。

### 本地测试

需要 `socat` 创建虚拟串口：

```bash
./script/dev-serial-tcp.sh      # 创建 /tmp/ttyV0 <-> TCP:9000
./out/build/x64/remoteSerial    # 另开终端启动服务
nc 127.0.0.1 9000               # 模拟串口设备
```

---

## REST API

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/ports` | 获取串口列表 |
| POST | `/api/port/open` | 打开串口 |
| POST | `/api/port/close` | 关闭串口 |
| POST | `/api/port/write` | 发送数据 |
| GET | `/api/port/status` | 查询串口状态 |
| POST | `/api/modbus/read` | Modbus 读操作（FC 0x01-0x04） |
| POST | `/api/modbus/write` | Modbus 写操作（FC 0x05/0x06/0x0F/0x10） |

WebSocket 端点：`/ws` — 串口接收数据实时推送到订阅客户端。

### 数据格式

发送数据请求体支持 `format` 字段：`hex` / `ascii` / `base64`（默认 `ascii`）。

### Modbus 请求示例

**读保持寄存器：**
```json
POST /api/modbus/read
{ "port": "/dev/ttyUSB0", "slave": 1, "function": 3, "address": 0, "quantity": 10, "timeout": 1000 }
```

**写单个寄存器：**
```json
POST /api/modbus/write
{ "port": "/dev/ttyUSB0", "slave": 1, "function": 6, "address": 0, "value": 12345, "timeout": 1000 }
```

---

## 技术栈

| 组件 | 技术 |
|------|------|
| 语言 | C++17 |
| HTTP/WS 服务 | libhv |
| 串口通信 | standalone Asio（`asio::serial_port`） |
| JSON | nlohmann/json |
| 日志 | spdlog |
| 构建 | CMake + Ninja |
| 前端 | 纯 HTML/CSS/JS，无框架依赖 |

---

## 项目结构

```
├── CMakeLists.txt              # 项目入口 + 源文件注册
├── CMakePresets.json            # CMake 预设（Ninja 生成器）
├── cmake/dependences.cmake     # FetchContent 依赖声明
├── src/
│   ├── main.cpp                # 入口，初始化 io_context + 信号处理
│   ├── http/
│   │   ├── http_server.h/cpp   # libhv HTTP/WS 服务封装，路由注册
│   ├── serial/
│   │   ├── serial_port.h/cpp   # 串口底层读写封装
│   │   └── serial_manager.h/cpp# 串口生命周期管理 + 并发控制
│   ├── modbus/
│   │   ├── modbus_rtu.h/cpp    # RTU 帧组装、CRC16、响应解析
│   │   └── modbus_manager.h/cpp# Modbus 事务管理（超时、异常处理）
│   └── utils/
│       ├── logger.h/cpp        # spdlog 封装
│       └── log_saver.h/cpp     # 日志文件持久化
├── web/
│   ├── index.html              # 界面：标签切换（串口 / Modbus）
│   ├── css/style.css           # 工业控制台主题样式
│   └── js/app.js               # 串口 + Modbus 交互逻辑
├── script/
│   ├── dev-serial-tcp.sh       # 虚拟串口 <-> TCP 桥接
│   └── dev-serial-udp.sh       # 虚拟串口 <-> UDP 桥接
└── doc/                        # 设计文档
```

---

## 开发规范

### 代码风格
- 命名：`snake_case` 函数和变量，`PascalCase` 类名，`kCamelCase` 常量
- 文件：一个类对应一个 `.h/.cpp` 对，文件名 `snake_case`
- 头文件：使用 `#pragma once`
- 日志：使用 `Logger::Info/Warn/Error`，不许 `printf/cout`

### 并发规则
- 串口写操作必须经过 `SerialManager` 互斥锁串行化
- 串口读数据由 `SerialManager` 广播到所有订阅该串口的 WebSocket 客户端
- Modbus 事务通过 `FeedResponse` 机制与串口数据流协调，无回调冲突

### 构建约束
- 新增源文件必须在 `CMakeLists.txt` 的 `add_executable` 中注册
- 新增第三方依赖统一在 `cmake/dependences.cmake` 中用 `FetchContent` 声明

---

## 许可证

[MIT](LICENSE)
