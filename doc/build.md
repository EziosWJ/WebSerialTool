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

## Windows 编译（Visual Studio）

在 Visual Studio 中打开 `CMakeLists.txt`，选择生成配置并编译。
