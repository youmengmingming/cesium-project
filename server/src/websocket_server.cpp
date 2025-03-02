#include "../include/websocket_server.h"
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
      connection_handler_(std::move(connection_handler)),
      writing_(false),
      is_open_(true) {
}

// WebSocket 会话析构函数
WebSocketSession::~WebSocketSession() {
    // 标记会话已关闭
    is_open_ = false;
    
    // 通知连接已关闭
    if (connection_handler_) {
        try {
            // 使用尝试，而不是直接使用shared_from_this()，以防在析构过程中已经无法获取shared_ptr
            connection_handler_(shared_from_this(), false);
        } catch (const std::exception& e) {
            std::cerr << "Error in ~WebSocketSession: " << e.what() << std::endl;
        }
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
                    self->connection_handler_(self, true);
                }

                // 开始读取消息
                self->doRead();
            }));
}

// 发送消息
void WebSocketSession::send(const std::string& message) {
    // 使用互斥锁保护队列访问
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 检查会话是否已关闭
        if (!is_open_) {
            std::cerr << "Trying to send to closed WebSocket session" << std::endl;
            return;
        }
        
        // 将消息添加到队列
        write_queue_.push(message);
        
        // 如果已经有写操作在进行，直接返回
        if (writing_) {
            return;
        }
    }
    
    // 开始处理写队列
    doWrite();
}

// 处理消息队列
void WebSocketSession::doWrite() {
    // 设置写入标志
    writing_ = true;
    
    std::string message;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (write_queue_.empty()) {
            writing_ = false;
            return;
        }
        // 使用std::move来避免不必要的复制，并确保原始字符串被清空
        message = std::move(write_queue_.front());
        write_queue_.pop();
    }
    
    // 确保WebSocket连接处于开放状态
    if (!ws_.is_open()) {
        std::cerr << "WebSocket is closed, cannot send message" << std::endl;
        writing_ = false;
        return;
    }
    
    try {
        // 创建一个共享指针来存储消息，确保在异步操作期间消息不会被销毁
        auto msg_ptr = std::make_shared<std::string>(std::move(message));
        
        // 发送消息到客户端
        ws_.async_write(
            net::buffer(*msg_ptr),
            beast::bind_front_handler(
                [self = shared_from_this(), msg_ptr](beast::error_code ec, std::size_t bytes_transferred) {
                    if (ec) {
                        std::cerr << "WebSocket write error: " << ec.message() << std::endl;
                        self->writing_ = false;
                        return;
                    }
                    
                    // 检查队列中是否还有更多消息
                    self->doWrite();
                }));
    } catch (const std::exception& e) {
        std::cerr << "Exception in WebSocketSession::doWrite: " << e.what() << std::endl;
        writing_ = false;
    }
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
                    // 创建消息的副本，避免在回调中使用buffer_的引用
                    std::string message = beast::buffers_to_string(self->buffer_.data());
                    self->buffer_.consume(self->buffer_.size());
                    
                    try {
                        // 直接传递会话对象自身
                        self->message_handler_(message, self);
                    } catch (const std::exception& e) {
                        std::cerr << "Error in message handler: " << e.what() << std::endl;
                    }
                }

                // 继续读取下一条消息
                self->doRead();
            }));
}

// 处理关闭
void WebSocketSession::onClose(beast::error_code ec) {
    // 标记会话已关闭
    is_open_ = false;
    
    if (ec == websocket::error::closed) {
        std::cout << "WebSocket connection closed normally" << std::endl;
    } else {
        std::cerr << "WebSocket error: " << ec.message() << std::endl;
    }

    // 关闭 WebSocket 连接
    beast::error_code close_ec;
    
    // 用互斥锁保护关闭操作
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 清空消息队列
        std::queue<std::string> empty;
        std::swap(write_queue_, empty);
    }
    
    // 只在连接仍然打开时尝试关闭
    if (ws_.is_open()) {
        try {
            ws_.close(websocket::close_code::normal, close_ec);
            if (close_ec) {
                std::cerr << "Error closing WebSocket: " << close_ec.message() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in WebSocketSession::onClose: " << e.what() << std::endl;
        }
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
    // 创建消息的副本，确保在多线程环境中安全
    std::string message_copy = message;
    
    // 创建会话集合的快照，在锁内复制指针但在锁外发送消息
    std::vector<std::shared_ptr<WebSocketSession>> session_snapshot;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        session_snapshot.reserve(sessions_.size());
        for (const auto& session : sessions_) {
            session_snapshot.push_back(session);
        }
    }
    
    // 在锁外发送消息给所有会话
    for (const auto& session : session_snapshot) {
        try {
            // 为每个会话传递消息的副本
            session->send(message_copy);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what() << std::endl;
        }
    }
}

// 向特定会话发送消息
void WebSocketServer::sendTo(const std::shared_ptr<WebSocketSession>& session, const std::string& message) {
    if (session) {
        try {
            // 创建消息的副本，确保在多线程环境中安全
            std::string message_copy = message;
            session->send(message_copy);
        } catch (const std::exception& e) {
            std::cerr << "Error sending message to specific session: " << e.what() << std::endl;
        }
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
                        [this](const std::string& message, const std::shared_ptr<WebSocketSession>& session) {
                            if (message_handler_) {
                                try {
                                    message_handler_(message, session);
                                } catch (const std::exception& e) {
                                    std::cerr << "Error in message handler: " << e.what() << std::endl;
                                }
                            }
                        },
                        [this](const std::shared_ptr<WebSocketSession>& session, bool connected) {
                            try {
                                if (connected) {
                                    // 这里不需要做任何事情，会话已经在下方添加到了sessions_集合
                                } else {
                                    // 断开连接时从集合中移除会话
                                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                                    auto it = sessions_.find(session);
                                    if (it != sessions_.end()) {
                                        sessions_.erase(it);
                                    }
                                }

                                if (connection_handler_) {
                                    connection_handler_(session, connected);
                                }
                            } catch (const std::exception& e) {
                                std::cerr << "Error in connection handler: " << e.what() << std::endl;
                            }
                        });

                    // 将会话添加到集合中
                    {
                        std::lock_guard<std::mutex> lock(sessions_mutex_);
                        sessions_.insert(session);
                    }

                    // 启动会话
                    try {
                        session->run();
                    } catch (const std::exception& e) {
                        std::cerr << "Error starting WebSocket session: " << e.what() << std::endl;
                        // 如果启动失败，从集合中移除会话
                        std::lock_guard<std::mutex> lock(sessions_mutex_);
                        sessions_.erase(session);
                    }
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