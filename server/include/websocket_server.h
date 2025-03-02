#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include <mutex>
#include <queue>
#include <atomic>

namespace cesium_server {

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// 前置声明WebSocketSession类
class WebSocketSession;

// WebSocket 消息处理器类型
using WebSocketMessageHandler = std::function<void(
    const std::string&, 
    const std::shared_ptr<WebSocketSession>&)>;

// WebSocket 连接处理器类型
using WebSocketConnectionHandler = std::function<void(
    const std::shared_ptr<WebSocketSession>&, 
    bool)>; // true for connect, false for disconnect

// WebSocket 会话类
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    explicit WebSocketSession(tcp::socket&& socket, 
                             WebSocketMessageHandler message_handler,
                             WebSocketConnectionHandler connection_handler);
    ~WebSocketSession();

    // 启动会话
    void run();

    // 发送消息
    void send(const std::string& message);
    
    // 获取WebSocket流的引用
    websocket::stream<beast::tcp_stream>& getStream() { return ws_; }

private:
    // 接收消息
    void doRead();

    // 处理关闭
    void onClose(beast::error_code ec);
    
    // 处理消息队列
    void doWrite();

    // WebSocket 流
    websocket::stream<beast::tcp_stream> ws_;

    // 消息缓冲区
    beast::flat_buffer buffer_;

    // 消息处理器
    WebSocketMessageHandler message_handler_;

    // 连接处理器
    WebSocketConnectionHandler connection_handler_;
    
    // 互斥锁，保护异步操作
    std::mutex mutex_;
    
    // 消息队列
    std::queue<std::string> write_queue_;
    
    // 指示是否有写操作正在进行
    std::atomic<bool> writing_{false};
    
    // 指示会话是否活跃
    std::atomic<bool> is_open_{true};
};

// WebSocket 服务器类
class WebSocketServer {
public:
    WebSocketServer(const std::string& address, unsigned short port, int threads = 1);
    ~WebSocketServer();

    // 启动服务器
    void run();

    // 停止服务器
    void stop();

    // 设置消息处理器
    void setMessageHandler(WebSocketMessageHandler handler);

    // 设置连接处理器
    void setConnectionHandler(WebSocketConnectionHandler handler);

    // 广播消息给所有客户端
    void broadcast(const std::string& message);
    
    // 向特定会话发送消息
    void sendTo(const std::shared_ptr<WebSocketSession>& session, const std::string& message);

private:
    // 接受新连接
    void doAccept();

    // 服务器地址和端口
    std::string address_;
    unsigned short port_;
    int num_threads_;

    // IO 上下文和接收器
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::vector<std::thread> threads_;

    // 消息处理器
    WebSocketMessageHandler message_handler_;

    // 连接处理器
    WebSocketConnectionHandler connection_handler_;

    // 活跃会话列表
    std::set<std::shared_ptr<WebSocketSession>> sessions_;
    std::mutex sessions_mutex_;

    // 服务器状态
    bool running_;
};

} // namespace cesium_server