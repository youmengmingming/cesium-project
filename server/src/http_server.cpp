#include "http_server.h"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

namespace cesium_server {

// HTTP 服务器构造函数
HttpServer::HttpServer(const std::string& address, unsigned short port, int threads)
    : address_(address), port_(port), num_threads_(threads), 
      ioc_(threads), acceptor_(net::make_strand(ioc_)), running_(false) {
    
    beast::error_code ec;

    // 打开接收器
    tcp::endpoint endpoint(net::ip::make_address(address), port);
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Error opening acceptor: " << ec.message() << std::endl;
        return;
    }

    // 允许地址重用
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Error setting reuse_address: " << ec.message() << std::endl;
        return;
    }

    // 绑定到端口
    acceptor_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Error binding to " << address << ":" << port << ": " << ec.message() << std::endl;
        return;
    }

    // 开始监听
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Error listening: " << ec.message() << std::endl;
        return;
    }

    std::cout << "HTTP server initialized on " << address << ":" << port << std::endl;
}

// 析构函数
HttpServer::~HttpServer() {
    stop();
}

// 启动服务器
void HttpServer::run() {
    if (running_) return;
    running_ = true;

    // 开始接受连接
    doAccept();

    // 启动工作线程
    threads_.reserve(num_threads_);
    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this] { ioc_.run(); });
    }

    std::cout << "HTTP server running with " << num_threads_ << " threads" << std::endl;
}

// 停止服务器
void HttpServer::stop() {
    if (!running_) return;
    running_ = false;

    // 停止接收器
    beast::error_code ec;
    acceptor_.close(ec);

    // 停止 IO 上下文
    ioc_.stop();

    // 等待所有线程完成
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();

    std::cout << "HTTP server stopped" << std::endl;
}

// 注册路由处理器
void HttpServer::registerHandler(const std::string& path, HttpRequestHandler handler) {
    handlers_[path] = handler;
    std::cout << "Registered handler for path: " << path << std::endl;
}

// 接受新连接
void HttpServer::doAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // 创建新的会话处理请求
                    std::thread(&HttpServer::handleRequest, this, std::move(socket)).detach();
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }

                // 如果服务器仍在运行，继续接受连接
                if (running_) {
                    doAccept();
                }
            }));
}

// 处理请求
void HttpServer::handleRequest(tcp::socket socket) {
    try {
        // 设置超时
        beast::flat_buffer buffer;
        socket.set_option(tcp::no_delay(true));

        // 读取请求
        http::request<http::string_body> req;
        http::read(socket, buffer, req);
        
        // 解析请求目标
        auto const& target_view = req.target();
        std::string target_string(target_view.begin(), target_view.end());
        auto handler = findHandler(target_string);
        
        http::response<http::string_body> res;
        if (handler) {
            res = handler(req, target_string);
        } else {
            // 默认 404 响应
            res = http::response<http::string_body>(
                http::status::not_found, req.version());
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            
            // 构建错误消息
            std::stringstream ss;
            ss << "The resource '" << target_string << "' was not found.";
            res.body() = ss.str();
            res.prepare_payload();
        }

        // 发送响应
        http::write(socket, res);

        // 如果不保持连接，关闭套接字
        if (!req.keep_alive()) {
            beast::error_code ec;
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
    }
}

// 查找路由处理器
HttpRequestHandler HttpServer::findHandler(const std::string& path) {
    // 精确匹配
    auto it = handlers_.find(path);
    if (it != handlers_.end()) {
        return it->second;
    }

    // 前缀匹配
    for (const auto& [prefix, handler] : handlers_) {
        if (path.find(prefix) == 0) {
            return handler;
        }
    }

    return nullptr;
}

} // namespace cesium_server