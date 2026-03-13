# 系统架构说明

## 模块划分

* **Web UI（Browser）**
  * 提供串口选择、参数设置、发送/接收数据
  * 通过 REST API 调用控制接口，通过 WebSocket 接收实时数据

* **C++ Server**
  * **HTTP/REST + WebSocket**：基于 libhv 实现，提供静态页面、API、实时推送
  * **SerialManager**：管理串口打开/关闭、读写、权限与并发控制
  * **SerialPort**：封装底层串口读写（可用 Boost.Asio、termios、Win32）

* **串口设备**
  * Linux：`/dev/ttyS*` / `/dev/ttyUSB*` / `/dev/ttyACM*`
  * Windows：`COM1` / `COM2` / …

## 并发与数据流

* **写操作串行化**：所有写请求通过 `SerialManager` 的互斥锁排他，避免多客户端竞争导致的混乱数据。
* **读数据广播**：串口收到的数据由 `SerialManager` 分发到所有订阅该串口的 WebSocket 客户端。

## 依赖与扩展点

* **网络层**：libhv（支持 HTTP/HTTPS、WebSocket）
* **串口层**：可选 Boost.Asio、libserialport、直接 termios/Win32 API
* **JSON**：nlohmann/json
* **日志**：spdlog
