# 构建说明

## 前提依赖

* CMake 3.10+
* C++17 编译器（GCC/Clang/MSVC）

### 可选依赖（后续功能）

* Boost (Asio)
* libhv
* spdlog
* nlohmann/json

## Linux 编译

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

可执行文件位于：

```
build/remoteSerial
```

## 运行

支持命令行参数：

```bash
./remoteSerial                               # 默认 8080，自动检测 web/ 目录
./remoteSerial --port 9090                   # 改端口
./remoteSerial --web-root /path/to/web       # 指定 web 目录
./remoteSerial --port 9090 --web-root ./web
```

Web 根目录自动检测顺序：`--web-root` 参数 > exe 同级 `web/` > exe 上级 `web/` > 回退 `web/`（相对 CWD）。

## 本地测试

使用 `socat` 创建虚拟串口进行测试。

### TCP 桥接

```bash
# 终端 1：创建虚拟串口 /tmp/ttyV0，桥接到 TCP 端口 9000
./script/dev-serial-tcp.sh [port]

# 终端 2：启动服务
./build/remoteSerial

# 终端 3：用 nc 模拟串口设备收发数据
nc 127.0.0.1 9000
```

服务端打开 `/tmp/ttyV0`，`nc` 发送的内容会转发到串口，串口写入的内容会显示在 `nc` 上。

### UDP 桥接

```bash
# 终端 1：虚拟串口 → UDP 双向桥接
./script/dev-serial-udp.sh [local-port] [remote-host] [remote-port]

# 发送数据到串口：
echo 'hello' | nc -u 127.0.0.1 <local-port>

# 接收串口数据：
nc -ul <remote-port>
```

### 模拟串口设备

脚本基于 `socat pty` 创建虚拟串口设备，路径固定为 `/tmp/ttyV0`。服务端通过 `ListPorts` 自动扫描 `/tmp/ttyV[0-9]*` 模式发现设备。

## Windows 编译（Visual Studio）

在 Visual Studio 中打开 `CMakeLists.txt`，选择生成配置并编译。
