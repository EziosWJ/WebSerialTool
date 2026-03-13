### 获取libhv库
下载更小（推荐）

浅克隆：非常适合嵌入式 buildroot / CI

```shell
git clone --depth 1 --branch v1.3.4 --single-branch https://github.com/ithewei/libhv.git libraries/sources/libhv
```
特点：只下载 一个 commit


#### 直观方便的管理方式

`FetchContent`获取

```shell
include(FetchContent)

FetchContent_Declare(
    libhv
    GIT_REPOSITORY https://github.com/ithewei/libhv.git
    GIT_TAG v1.3.4
)

FetchContent_MakeAvailable(libhv)
```