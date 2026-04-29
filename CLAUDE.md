# WebSerialTool — CLAUDE.md

## 项目概述

跨平台 Web 串口调试工具，C++17 后端提供 HTTP/WebSocket 服务，前端浏览器访问，支持串口配置、收发数据、日志保存。

## 技术栈

- **语言**: C++17
- **框架**: libhv（HTTP + WebSocket 服务端）
- **串口**: standalone Asio（`asio::serial_port`）
- **JSON**: nlohmann/json
- **日志**: spdlog
- **构建**: CMake ≥ 3.10
- **前端**: 纯 HTML/CSS/JS，无框架依赖

## 目录结构

```
├── CMakeLists.txt          # 项目入口 + 源文件注册
├── cmake/dependences.cmake # FetchContent 依赖声明
├── src/
│   ├── main.cpp            # 入口，初始化 io_context + 信号处理
│   ├── http/
│   │   ├── http_server.h/cpp   # libhv 服务封装，路由注册
│   ├── serial/
│   │   ├── serial_port.h/cpp   # 串口底层读写封装
│   │   └── serial_manager.h/cpp # 串口生命周期管理 + 并发控制
│   └── utils/
│       ├── logger.h/cpp        # spdlog 封装
│       └── log_saver.h/cpp     # 日志保存
├── web/
│   ├── index.html
│   ├── css/style.css
│   └── js/app.js
├── doc/                    # 设计文档
├── script/                 # 开发辅助脚本
│   ├── dev-serial-tcp.sh   # 虚拟串口 ←→ TCP 桥接
│   └── dev-serial-udp.sh   # 虚拟串口 ←→ UDP 桥接
└── build/                  # 构建输出
```

## 构建 & 运行

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
./remoteSerial            # 默认端口 8080，自动检测 web/
./remoteSerial --port 9090 --web-root /path/to/web
```

Web 根目录自动检测：`--web-root` 参数 > exe 同级 `web/` > exe 上级 `web/` > 回退 `web/`。

### 本地测试

需要 `socat`，使用脚本创建虚拟串口：

```bash
./script/dev-serial-tcp.sh          # 创建 /tmp/ttyV0 ←→ TCP:9000
./build/remoteSerial                # 另开终端启动服务
nc 127.0.0.1 9000                   # nc 模拟串口设备
```

## REST API

| 方法   | 路径                | 说明         |
| ------ | ------------------- | ------------ |
| GET    | `/api/ports`        | 获取串口列表 |
| POST   | `/api/port/open`    | 打开串口     |
| POST   | `/api/port/close`   | 关闭串口     |
| POST   | `/api/port/write`   | 发送数据     |
| GET    | `/api/port/status`  | 查询串口状态 |

WebSocket 端点 `/ws`，串口接收数据实时推送到订阅客户端。

## 数据格式

发送数据请求体支持 `format` 字段：`hex` / `ascii` / `base64`（默认 `ascii`）。

## 开发规范

### 代码风格
- 命名：`snake_case` 函数和变量，`PascalCase` 类名，`kCamelCase` 常量
- 文件：一个类对应一个 `.h/.cpp` 对，文件名 `snake_case`
- 头文件：使用 `#pragma once`
- 日志：使用 `Logger::Info/Warn/Error`，不许 `printf/cout`

### 并发规则
- 串口写操作必须经过 `SerialManager` 互斥锁串行化
- 串口读数据由 `SerialManager` 广播到所有订阅该串口的 WebSocket 客户端
- 每个 WebSocket 连接维护独立会话上下文

### 错误处理
- API 返回统一格式：`{"code": int, "msg": string}`
- 串口操作失败必须向上层返回错误码和描述
- 覆盖串口拔插、打开失败、读写异常等场景，向客户端推送状态变更

### 构建约束
- 新增源文件必须在 `CMakeLists.txt` 的 `add_executable` 中注册
- 新增第三方依赖统一在 `cmake/dependences.cmake` 中用 `FetchContent` 声明

### 安全意识
- 勿在代码中硬编码密码/token
- HTTPS 和登录认证为可选增强特性，不影响核心功能
