# WebSerialTool

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20Embedded-lightgrey.svg)]()

[English](README.md) | [中文](README_CN.md)

> A lightweight, cross-platform web-based serial port debugging tool with Modbus RTU support.
> Access and control your serial devices directly from a browser — no desktop software required.

---

## Features

- **Web UI** — Industrial control panel aesthetic, debug serial ports via any modern browser
- **Remote Access** — Connect to devices over the network (e.g. `http://192.168.1.10:8080`)
- **Multi-Platform** — Linux, Windows, Embedded Linux (Buildroot / OpenWRT / Yocto)
- **Full Serial Configuration** — Baud rate, data bits, stop bits, parity
- **Multiple Data Formats** — Send/receive in ASCII, HEX, or Base64
- **Real-Time Data** — WebSocket push, no polling
- **Modbus RTU** — Dedicated protocol panel with all 8 function codes (0x01-0x10), response table, auto-polling
- **Log Saving** — Export communication logs for analysis
- **Zero Runtime Dependencies** — Statically linkable

---

## Screenshots

```
┌─────────────────────────────────────────────────────┐
│  △ SERIAL          [SERIAL] [MODBUS]                │
│    DEBUG TOOL       Status: ONLINE                  │
├─────────────────────────────────────────────────────┤
│  PORT           │  CONSOLE                          │
│  [/dev/ttyUSB0▾]│  10:23:45.123 RX /dev/ttyUSB0 ...│
│  PARAMETERS     │  10:23:45.456 TX /dev/ttyUSB0 ...│
│  BAUD  115200   │  10:23:46.789 RX /dev/ttyUSB0 ...│
│  DATA  8        │                                   │
│  STOP  1        │                                   │
│  PARITY NONE    │                                   │
│  [OPEN] [CLOSE] │                                   │
└─────────────────────────────────────────────────────┘
```

---

## Quick Start

### Prerequisites

- CMake >= 3.14
- C++17 compiler (GCC >= 7 or MSVC >= 2019)
- Git
- Ninja (recommended)

### Build

```bash
git clone https://github.com/EziosWJ/WebSerialTool.git
cd WebSerialTool

# Using CMake Presets (recommended)
cmake --preset x64
cmake --build out/build/x64

# Or traditional way
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
./out/build/x64/remoteSerial                     # Default port 8080
./out/build/x64/remoteSerial --port 9090          # Custom port
./out/build/x64/remoteSerial --web-root /path/to/web
```

Open `http://localhost:8080` in your browser.

> Web root auto-detection priority: `--web-root` flag > `web/` next to executable > `web/` in parent directory > fallback `web/` in CWD.

### Local Testing

Requires `socat` for virtual serial ports:

```bash
./script/dev-serial-tcp.sh      # Creates /tmp/ttyV0 <-> TCP:9000
./out/build/x64/remoteSerial    # Start server in another terminal
nc 127.0.0.1 9000               # Simulate serial device
```

---

## REST API

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/ports` | List available serial ports |
| POST | `/api/port/open` | Open a serial port |
| POST | `/api/port/close` | Close a serial port |
| POST | `/api/port/write` | Send data to serial port |
| GET | `/api/port/status` | Query port status |
| POST | `/api/modbus/read` | Modbus read operation (FC 0x01-0x04) |
| POST | `/api/modbus/write` | Modbus write operation (FC 0x05/0x06/0x0F/0x10) |

WebSocket endpoint: `/ws` — real-time serial data push to subscribed clients.

### Data Format

Send request body supports `format` field: `hex` / `ascii` / `base64` (default: `ascii`).

### Modbus Request Examples

**Read holding registers:**
```json
POST /api/modbus/read
{ "port": "/dev/ttyUSB0", "slave": 1, "function": 3, "address": 0, "quantity": 10, "timeout": 1000 }
```

**Write single register:**
```json
POST /api/modbus/write
{ "port": "/dev/ttyUSB0", "slave": 1, "function": 6, "address": 0, "value": 12345, "timeout": 1000 }
```

---

## Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++17 |
| HTTP/WS Server | libhv |
| Serial Port | standalone Asio (`asio::serial_port`) |
| JSON | nlohmann/json |
| Logging | spdlog |
| Build | CMake + Ninja |
| Frontend | Vanilla HTML/CSS/JS, no framework |

---

## Project Structure

```
├── CMakeLists.txt              # Build entry + source registration
├── CMakePresets.json            # CMake presets (Ninja generator)
├── cmake/dependences.cmake     # FetchContent dependency declarations
├── src/
│   ├── main.cpp                # Entry point, io_context + signal handling
│   ├── http/
│   │   ├── http_server.h/cpp   # libhv HTTP/WS server, route registration
│   ├── serial/
│   │   ├── serial_port.h/cpp   # Low-level serial read/write wrapper
│   │   └── serial_manager.h/cpp# Serial lifecycle management + concurrency
│   ├── modbus/
│   │   ├── modbus_rtu.h/cpp    # RTU frame assembly, CRC16, response parsing
│   │   └── modbus_manager.h/cpp# Modbus transaction management (timeout, exceptions)
│   └── utils/
│       ├── logger.h/cpp        # spdlog wrapper
│       └── log_saver.h/cpp     # Log file persistence
├── web/
│   ├── index.html              # UI: tab switch (Serial / Modbus)
│   ├── css/style.css           # Industrial control panel theme
│   └── js/app.js               # Serial + Modbus interaction logic
├── script/
│   ├── dev-serial-tcp.sh       # Virtual serial <-> TCP bridge
│   └── dev-serial-udp.sh       # Virtual serial <-> UDP bridge
└── doc/                        # Design documents
```

---

## License

[MIT](LICENSE)
