#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include "thread_pool.h"

namespace cesium_server {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// HTTP 请求处理器类型
using HttpRequestHandler = std::function<http::response<http::string_body>(
    const http::request<http::string_body>&, 
    const std::string&)>;

// HTTP 服务器类
class HttpServer {
public:
    HttpServer(const std::string& address, unsigned short port, int threads = 1);
    ~HttpServer();

    // 启动服务器
    void run();

    // 停止服务器
    void stop();

    // 注册路由处理器
    void registerHandler(const std::string& path, HttpRequestHandler handler);

private:
    // 接受新连接
    void doAccept();

    // 处理请求
    void handleRequest(tcp::socket socket);

    // 查找路由处理器
    HttpRequestHandler findHandler(const std::string& path);

    // 服务器地址和端口
    std::string address_;
    unsigned short port_;
    int num_threads_;

    // IO 上下文和接收器
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::unique_ptr<ThreadPool> thread_pool_;

    // 路由表
    std::map<std::string, HttpRequestHandler> handlers_;

    // 服务器状态
    bool running_;
};

} // namespace cesium_server