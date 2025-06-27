# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称和语言
project(CA CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(storage
    storage/test.cpp
    storage/base64.cpp
    storage/bundle.cpp
)

# 包含头文件目录
target_include_directories(storage PRIVATE log)
target_include_directories(storage PRIVATE storage)

#包含库文件
target_link_libraries(storage PRIVATE event stdc++fs)