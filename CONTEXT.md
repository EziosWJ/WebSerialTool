# WebSerialTool

跨平台 Web 串口调试工具，通过浏览器远程操作串口设备，支持原生串口数据收发和 Modbus 协议调试。

## Language

**串口 (Port)**:
操作系统分配的串行通信设备（如 `/dev/ttyUSB0`、`COM3`）。一个端口同一时刻只允许一个配置（波特率、数据位等）。
_Avoid_: 端口、serial port、COM port

**连接 (Connection)**:
一个 WebSocket 会话与一个或多个串口的订阅关系。连接断开时自动清理订阅。
_Avoid_: 会话、session

**订阅 (Subscribe)**:
WebSocket 客户端注册接收某个串口的数据推送。空订阅集表示接收所有串口数据。
_Avoid_: 监听、绑定

**Modbus 事务 (Transaction)**:
一次完整的 Modbus 请求-响应交互：主站发送请求帧 → 从站返回响应帧。包含超时和异常处理。
_Avoid_: Modbus 请求、Modbus 操作

**从站 (Slave)**:
Modbus 网络中响应请求的设备，通过地址（1-247）标识。
_Avoid_: 设备、device、server

**功能码 (Function Code)**:
Modbus 操作类型标识（0x01-0x10）。分为读操作（0x01-0x04）和写操作（0x05/0x06/0x0F/0x10）。

**寄存器 (Register)**:
Modbus 设备中的 16 位数据存储单元，通过地址寻址。分为保持寄存器（可读写）和输入寄存器（只读）。

**线圈 (Coil)**:
Modbus 设备中的 1 位布尔存储单元。分为线圈（可读写）和离散输入（只读）。

## Relationships

- 一个 **串口** 同时只能被一组**连接**订阅
- 一个 **Modbus 事务** 绑定到一个**串口**，使用该串口的物理参数（波特率等）
- 一个 **从站** 通过一个**串口**通信，可以包含多个**寄存器**和**线圈**
- 一个 **功能码** 定义了对**寄存器**或**线圈**的操作类型（读/写）

## Example dialogue

> **Dev**: "用户想读从站 2 的保持寄存器，起始地址 100，数量 5。前端怎么发？"
> **Domain expert**: "POST `/api/modbus/read`，body 里带 `slave: 2, function: 0x03, address: 100, quantity: 5`。后端组装 RTU 帧，算 CRC，发到串口，等响应，解析后返回结构化数据。"

> **Dev**: "从站返回异常码 0x82 是什么意思？"
> **Domain expert**: "功能码最高位置 1 表示异常响应，0x82 = 0x02 + 0x80，即读离散输入的异常。低 7 位 0x02 是原始功能码，异常码在下一个字节，比如 0x02 表示非法数据地址。"

## Flagged ambiguities

- "端口" 在中文里既指串口设备（Port）也指网络端口（如 8080）。统一用"串口"指串行设备，"端口"指网络端口。
