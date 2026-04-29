# Bug: io_context 线程提前退出导致串口读数据无法回显

## 现象

- Web 发送 → 串口输出：正常
- 串口发送 → Web 显示：无响应

## 根因

`io_context` 工作线程在注册异步操作前已退出。

```cpp
// main.cpp (修复前)
std::thread io_thread([&io_context]() {
    io_context.run();  // 此时无待处理异步操作，立即返回
});
```

`io_context.run()` 在没有异步操作时会立即返回。而 `async_read_some` 是在收到 HTTP 请求 `OpenPort` 后才注册，此时 `io_thread` 已结束，读完成回调无人处理。

同步写 `asio::write` 不依赖 `io_context` 线程，所以不受影响。

## 修复

使用 `asio::make_work_guard` 保持 `io_context` 活跃：

```cpp
// main.cpp (修复后)
auto work_guard = asio::make_work_guard(io_context);

std::thread io_thread([&io_context]() {
    io_context.run();   // work_guard 防止提前退出
});

// shutdown 时
work_guard.reset();     // 释放 work_guard，允许 io_context 退出
io_context.stop();
```

## 改动

- `src/main.cpp`：新增 `work_guard`，shutdown 时 `reset()`
