cmake_minimum_required(VERSION 3.10)
project(cesium_server VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#设置Boost库根目录
set(BOOST_ROOT "D:/thirdPart/boost/boost1.87.0")
set(OPENSSL_ROOT_DIR "D:/thirdPart/openssl/openssl3.4.1/debug")
set(MYSQL_ROOT_DIR "C:/Program Files/MySQL/MySQL Server 8.0/")
#set(SQLITE3_ROOT_DIR "D:/thirdPart/sqlite/sqlite3.42.0")

#禁用搜索系统路径
set(Boost_NO__SYSTEM_PATHS_ON)
set(OpenSSL_NO__SYSTEM_PATHS_ON)
set(MySQL_NO_SYSTEM_PATHS ON)
#set(SQLite3_NO_SYSTEM_PATHS ON)

# 查找必要的包
find_package(Boost 1.87.0 REQUIRED COMPONENTS system thread json)
find_package(OpenSSL REQUIRED)
#find_package(MySQL REQUIRED)
#find_package(SQLite3 REQUIRED)

set(OPENSSL_CRYPTO_LIBRARY "D:/thirdPart/openssl/openssl3.4.1/debug/lib/libcrypto.lib")
set(OPENSSL_SSL_LIBRARY "D:/thirdPart/openssl/openssl3.4.1/debug/lib/libssl.lib")

set(Boost_INCLUDE_DIRS "D:/thirdPart/boost/boost1.87.0")
set(MySQL_INCLUDE_DIRS "${MYSQL_ROOT_DIR}/include")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${MySQL_INCLUDE_DIRS}
)

link_directories("C:/Program Files/MySQL/MySQL Server 8.0/lib")

#添加头文件
file(GLOB_RECURSE HEAD "include/*.h","include/.hpp")

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(cesium_server ${SOURCES})

# 链接库
target_link_libraries(cesium_server
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${MySQL_LIBRARIES}
    ws2_32
    wsock32
    libmysql
)

# 安装目标
install(TARGETS cesium_server DESTINATION bin)