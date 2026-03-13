# REST API 设计

## 1. 获取串口列表

**GET** `/api/ports`

响应示例：

```json
{
  "ports": [
    "ttyUSB0",
    "ttyUSB1"
  ]
}
```

---

## 2. 打开串口

**POST** `/api/port/open`

请求体：

```json
{
  "port": "ttyUSB0",
  "baud": 115200,
  "databits": 8,
  "stopbits": 1,
  "parity": "none"
}
```

响应示例（成功）：

```json
{
  "code": 0,
  "msg": "ok"
}
```

---

## 3. 关闭串口

**POST** `/api/port/close`

请求体：

```json
{
  "port": "ttyUSB0"
}
```

---

## 4. 发送数据

**POST** `/api/port/write`

请求体：

```json
{
  "port": "ttyUSB0",
  "data": "48656c6c6f",
  "format": "hex"
}
```

* `format` 可选，支持：`hex` / `ascii` / `base64`（默认 `ascii`）。

---

## 5. 状态查询（可选）

**GET** `/api/port/status?port=ttyUSB0`

响应示例：

```json
{
  "port": "ttyUSB0",
  "opened": true,
  "baud": 115200
}
```
