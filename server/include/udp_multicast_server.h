#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

namespace net = boost::asio;
using udp = boost::asio::ip::udp;

// UDP组播消息处理器类型
using UdpMessageHandler = std::function<void(const std::string&, const boost::asio::ip::udp::endpoint&)>;

// UDP组播服务器类
class UdpMulticastServer {
public:
    UdpMulticastServer(const std::string& multicast_address, unsigned short port,
                      const std::string& listen_address = "0.0.0.0");
    ~UdpMulticastServer();

    // 启动服务器
    void run();

    // 停止服务器
    void stop();

    // 发送组播消息
    void sendMessage(const std::string& message);

    // 设置消息处理器
    void setMessageHandler(UdpMessageHandler handler);

private:
    // 接收消息
    void doReceive();

    // 处理接收到的消息
    void handleMessage(const std::string& message, const boost::asio::ip::udp::endpoint& sender);

    // IO上下文
    net::io_context io_context_;

    // 工作守卫
    std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;

    // 套接字
    udp::socket socket_;

    // 组播地址
    udp::endpoint multicast_endpoint_;

    // 接收缓冲区
    std::vector<char> recv_buffer_;

    // 消息处理器
    UdpMessageHandler message_handler_;

    // IO线程
    std::thread io_thread_;

    // 运行标志
    std::atomic<bool> running_;

    // 互斥锁
    std::mutex mutex_;
};
