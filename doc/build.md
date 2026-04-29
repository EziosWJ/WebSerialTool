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

## Windows 编译（Visual Studio）

在 Visual Studio 中打开 `CMakeLists.txt`，选择生成配置并编译。
