# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置测试项目名称
project(cesium_server_tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system thread)

# 添加测试可执行文件
add_executable(test_server test_server.cpp)

# 链接Boost库
target_link_libraries(test_server
    PRIVATE
    Boost::system
    Boost::thread
)

# 包含头文件目录
target_include_directories(test_server
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 添加测试
add_test(NAME server_test COMMAND test_server)