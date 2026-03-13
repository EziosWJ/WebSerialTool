cmake_minimum_required(VERSION 3.10.0)
project(remoteSerial VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(remoteSerial
    src/main.cpp
    src/serial/serial_port.cpp
    src/serial/serial_manager.cpp
    src/http/http_server.cpp
    src/utils/logger.cpp
)

target_include_directories(remoteSerial PRIVATE src)

