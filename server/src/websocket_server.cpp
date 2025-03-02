#include "websocket_server.h"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace cesium_server {

// WebSocket 会话构造函数
WebSocketSession::WebSocketSession(
    tcp::socket&& socket,
    WebSocketMessageHandler message_handler,
    WebSocketConnectionHandler connection_handler)
    : ws_(std::move(socket)),
      message_handler_(std::move(message_handler)),
      connection_handler_(std::move(connection_handler)) {
}

// WebSocket 会话析构函数
WebSocketSession::~WebSocketSession() {
    // 通知连接已关闭
    if (connection_handler_) {
        std::shared_ptr<websocket::stream<beast::tcp_stream>> ws_ptr = 
            std::shared_ptr<websocket::stream<beast::tcp_stream>>(&ws_, [](auto*){});
        connection_handler_(ws_ptr, false);
    }
}

// 启动会话
void WebSocketSession::run() {
    // 设置 WebSocket 选项
    ws_.set_option(websocket::stream_base::timeout::suggested(
        beast::role_type::server));

    // 设置最大消息大小
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res) {
            res.set(boost::beast::http::field::server, 
                    std::string(BOOST_BEAST_VERSION_STRING) + " cesium-server");
        }));

    // 接受 WebSocket 握手
    ws_.async_accept(
        beast::bind_front_handler(
            [self = shared_from_this()](beast::error_code ec) {
                if (ec) {
                    std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
                    return;
                }

                // 通知连接已建立
                if (self->connection_handler_) {
                    std::shared_ptr<websocket::stream<beast::tcp_stream>> ws_ptr = 
                        std::shared_ptr<websocket::stream<beast::tcp_stream>>(&self->ws_, [](auto*){});
                    self->connection_handler_(ws_ptr, true);
                }

                // 开始读取消息
                self->doRead();
            }));
}

// 发送消息
void WebSocketSession::send(const std::string& message) {
    // 发送消息到客户端
    ws_.async_write(
        net::buffer(message),
        beast::bind_front_handler(
            [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "WebSocket write error: " << ec.message() << std::endl;
                    return;
                }
            }));
}

// 接收消息
void WebSocketSession::doRead() {
    // 异步读取消息
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    self->onClose(ec);
                    return;
                }

                // 处理消息
                if (self->message_handler_) {
                    std::string message = beast::buffers_to_string(self->buffer_.data());
                    self->buffer_.consume(self->buffer_.size());
                    std::shared_ptr<websocket::stream<beast::tcp_stream>> ws_ptr = 
                        std::shared_ptr<websocket::stream<beast::tcp_stream>>(&self->ws_, [](auto*){});
                    self->message_handler_(message, ws_ptr);
                }

                // 继续读取下一条消息
                self->doRead();
            }));
}

// 处理关闭
void WebSocketSession::onClose(beast::error_code ec) {
    if (ec == websocket::error::closed) {
        std::cout << "WebSocket connection closed normally" << std::endl;
    } else {
        std::cerr << "WebSocket error: " << ec.message() << std::endl;
    }

    // 关闭 WebSocket 连接
    beast::error_code close_ec;
    ws_.close(websocket::close_code::normal, close_ec);
    if (close_ec) {
        std::cerr << "Error closing WebSocket: " << close_ec.message() << std::endl;
    }
}

// WebSocket 服务器构造函数
WebSocketServer::WebSocketServer(const std::string& address, unsigned short port, int threads)
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

    std::cout << "WebSocket server initialized on " << address << ":" << port << std::endl;
}

// WebSocket 服务器析构函数
WebSocketServer::~WebSocketServer() {
    stop();
}

// 启动服务器
void WebSocketServer::run() {
    if (running_) return;
    running_ = true;

    // 开始接受连接
    doAccept();

    // 启动工作线程
    threads_.reserve(num_threads_);
    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this] { ioc_.run(); });
    }

    std::cout << "WebSocket server running with " << num_threads_ << " threads" << std::endl;
}

// 停止服务器
void WebSocketServer::stop() {
    if (!running_) return;
    running_ = false;

    // 停止接收器
    beast::error_code ec;
    acceptor_.close(ec);

    // 关闭所有会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.clear();
    }

    // 停止 IO 上下文
    ioc_.stop();

    // 等待所有线程完成
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();

    std::cout << "WebSocket server stopped" << std::endl;
}

// 设置消息处理器
void WebSocketServer::setMessageHandler(WebSocketMessageHandler handler) {
    message_handler_ = std::move(handler);
}

// 设置连接处理器
void WebSocketServer::setConnectionHandler(WebSocketConnectionHandler handler) {
    connection_handler_ = std::move(handler);
}

// 广播消息给所有客户端
void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    for (auto& session : sessions_) {
        session->send(message);
    }
}

// 接受新连接
void WebSocketServer::doAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // 创建新的 WebSocket 会话
                    auto session = std::make_shared<WebSocketSession>(
                        std::move(socket),
                        [this](const std::string& message, const std::shared_ptr<websocket::stream<beast::tcp_stream>>& ws_ptr) {
                            if (message_handler_) {
                                message_handler_(message, ws_ptr);
                            }
                        },
                        [this](const std::shared_ptr<websocket::stream<beast::tcp_stream>>& ws_ptr, bool connected) {
                            if (connected) {
                                // 实现连接处理
                                std::lock_guard<std::mutex> lock(sessions_mutex_);
                                // 这里的session会在WebSocketSession构造函数中自动添加到sessions_
                            } else {
                                // 断开连接处理已经在WebSocketSession析构函数中处理
                            }

                            if (connection_handler_) {
                                connection_handler_(ws_ptr, connected);
                            }
                        });

                    // 将会话添加到集合中
                    {
                        std::lock_guard<std::mutex> lock(sessions_mutex_);
                        sessions_.insert(session);
                    }

                    // 启动会话
                    session->run();
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }

                // 如果服务器仍在运行，继续接受连接
                if (running_) {
                    doAccept();
                }
            }));
}

} // namespace cesium_server