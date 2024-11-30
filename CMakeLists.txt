# 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Server)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置源文件和头文件目录
include_directories(include)

# 设置源文件
set(SOURCES
    src/main.cpp
    src/User.cpp
    src/LoginUserMap.cpp
    src/Message.cpp
)

# 可执行文件名称
add_executable(Server ${SOURCES})