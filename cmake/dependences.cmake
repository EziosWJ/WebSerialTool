# FetchContent缓存
# set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/.deps)
# libhv
include(FetchContent)

FetchContent_Declare(
    libhv
    GIT_REPOSITORY https://github.com/ithewei/libhv.git
    GIT_TAG v1.3.4
)

FetchContent_MakeAvailable(libhv)

target_link_libraries(${PROJECT_NAME} PRIVATE hv)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(json)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)

FetchContent_MakeAvailable(spdlog)

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# standone Asio
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)

FetchContent_MakeAvailable(asio)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${asio_SOURCE_DIR}/asio/include
)

# 定义ASIO_STANDALONE宏，告诉编译器我们使用的是standalone Asio
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ASIO_STANDALONE
)