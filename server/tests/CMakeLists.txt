# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置测试项目名称
project(cesium_server_tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加UTF-8编译支持
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置三方库路径
set(ThirdPart_DIR "D:/thirdPart")

# 设置Boost库根目录
set(BOOST_ROOT "D:/thirdPart/boost/boost1.87.0")
set(Boost_NO_SYSTEM_PATHS ON)

# 设置ZeroMQ库根目录
set(ZMQ_ROOT_DIR "D:/thirdPart/zmq/")
set(CPPZMQ_ROOT_DIR "D:/thirdPart/cppzmq/")

# 设置gRPC库根目录
set(GRPC_ROOT_DIR "${ThirdPart_DIR}/grpc")
set(PROTOBUF_ROOT_DIR "${ThirdPart_DIR}/protobuf")

# 设置SQLite库根目录
set(SQLITE3_ROOT_DIR "D:/thirdPart/sqlite")

# 设置spdlog库根目录
set(SPDLOG_ROOT_DIR "D:/thirdPart/spdlog/")

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system thread json)

# 添加测试可执行文件
add_executable(test_server test_server.cpp)
add_executable(test_grpc_service test_grpc_service.cpp)

# ZeroMQ库设置
set(ZMQ_INCLUDE_DIRS "${ZMQ_ROOT_DIR}/include")
set(ZMQ_LIBRARIES "${ZMQ_ROOT_DIR}/lib/Debug/libzmq-mt-gd-4_0_10.lib")

# gRPC库设置
set(GRPC_INCLUDE_DIRS "${GRPC_ROOT_DIR}/include")
set(PROTOBUF_INCLUDE_DIRS "${PROTOBUF_ROOT_DIR}/include")
set(GRPC_LIBRARIES 
    "${GRPC_ROOT_DIR}/lib/grpc++.lib"
    "${GRPC_ROOT_DIR}/lib/grpc.lib"
    "${GRPC_ROOT_DIR}/lib/gpr.lib"
    "${PROTOBUF_ROOT_DIR}/lib/libprotobuf.lib"
)

# 包含头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
    ${CPPZMQ_ROOT_DIR}
    ${GRPC_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${ThirdPart_DIR}/sqlite/include
    ${SPDLOG_ROOT_DIR}/include
)

# 链接库
target_link_libraries(test_server
    PRIVATE
    ${Boost_LIBRARIES}
    ${ZMQ_LIBRARIES}
    ws2_32
    wsock32
    "${ThirdPart_DIR}/sqlite/lib/sqlite3.lib"
    "${SPDLOG_ROOT_DIR}/lib/spdlog.lib"
)

target_link_libraries(test_grpc_service
    PRIVATE
    ${Boost_LIBRARIES}
    ${GRPC_LIBRARIES}
    ws2_32
    wsock32
)

# 添加测试
add_test(NAME server_test COMMAND test_server)
add_test(NAME grpc_service_test COMMAND test_grpc_service)