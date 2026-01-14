cmake_minimum_required(VERSION 4.0.3)
project(lcy LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认安装到系统目录
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "安装目录" FORCE)
endif()

# 设置头文件包含路径
include_directories(${CMAKE_SOURCE_DIR}/../)

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -Wall -Wextra -Wno-error=terminate)
else()
    add_compile_options(-g -O3 -DNDEBUG)
endif()

# 添加子目录
add_subdirectory(asio)
add_subdirectory(protocol)
add_subdirectory(rpc)
