# WebSerialTool

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)]
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20Embedded-lightgrey.svg)]

> **A lightweight, cross-platform web-based serial port debugging tool.**  
> Access and control your serial devices directly from a browser—no desktop software required.

---

## 🌟 Features

- ✅ **Web UI** – Debug serial ports via any modern browser
- 🌐 **Remote Access** – Connect to devices over the network (e.g., `http://192.168.1.10:8080`)
- 🔌 **Multi-Platform Support**: Linux, Windows, and Embedded Linux (Buildroot/OpenWRT/Yocto)
- ⚙️ **Full Serial Configuration**: Baud rate, data bits, stop bits, parity
- 📤 **Multiple Data Formats**: Send/receive in **ASCII**, **HEX**, or **Base64**
- 📡 **Real-Time Data** via WebSocket (no polling!)
- 📁 **Log Saving** – Export communication logs for analysis
- 🧱 **Zero External Dependencies** at runtime (statically linkable)

---

## 🖥️ Use Cases

- 🔧 **Embedded Device Debugging**: Deploy on a Raspberry Pi or router to debug UART sensors.
- 🏭 **Industrial Automation**: Monitor RS485/PLC devices from a central dashboard.
- 💻 **Replace Desktop Tools**: No more SecureCRT or SSCOM—use a browser instead.
- 👥 **Team Collaboration**: Multiple engineers can view the same serial stream simultaneously.

---

## 🚀 Quick Start

### Run Prebuilt Binary (Linux x86_64)

> *(Prebuilt binaries coming soon! For now, build from source.)*

### Build from Source

#### Prerequisites
- CMake ≥ 3.14
- C++17 compiler (GCC ≥ 7 or MSVC ≥ 2019)
- Git

#### Steps

```bash
git clone https://github.com/EziosWJ/WebSerialTool.git
cd WebSerialTool
mkdir build && cd build
cmake ..
make -j $(nproc)
```

#### Run

```bash
./build/remoteSerial                        # 默认端口 8080，自动检测 web/ 目录
./build/remoteSerial --port 9090            # 指定端口
./build/remoteSerial --web-root /path/to/web
```

打开浏览器访问 `http://localhost:8080`。

> Web 根目录自动检测优先级：`--web-root` 参数 > exe 同级 `web/` > exe 上级 `web/` > 回退 CWD 下 `web/`。