#include "../include/udp_multicast_server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// 构造函数
UdpMulticastServer::UdpMulticastServer(const std::string& multicast_address, unsigned short port,
                                     const std::string& listen_address)
    : multicast_endpoint_(net::ip::make_address(multicast_address), port),
      recv_buffer_(1024),
      socket_(net::make_strand(io_context_)),
      running_(false) {
    try {
        // 创建UDP套接字
        socket_.open(udp::v4());
        
        // 设置套接字选项
        socket_.set_option(udp::socket::reuse_address(true));
        
        // 绑定到指定端口
        socket_.bind(udp::endpoint(net::ip::address::from_string(listen_address), port));
        
        // 加入组播组
        socket_.set_option(net::ip::multicast::join_group(
            net::ip::make_address(multicast_address)));
        
        std::cout << "UDP Multicast Server initialized on " << listen_address 
                  << ":" << port << " (group: " << multicast_address << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "UDP Multicast Server initialization error: " << e.what() << std::endl;
        throw;
    }
}

// 析构函数
UdpMulticastServer::~UdpMulticastServer() {
    stop();
}

// 启动服务器
void UdpMulticastServer::run() {
    if (running_) {
        return;
    }
    
    running_ = true;
    
    // 创建工作守卫以保持io_context运行
    work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(
        io_context_.get_executor());
    
    // 开始接收消息
    doReceive();
    
    // 启动IO线程
    io_thread_ = std::thread([this]() {
        try {
            io_context_.run();
        }
        catch (const std::exception& e) {
            std::cerr << "UDP Multicast Server IO error: " << e.what() << std::endl;
        }
    });
    
    std::cout << "UDP Multicast Server running" << std::endl;
}

// 停止服务器
void UdpMulticastServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 移除工作守卫以允许io_context退出
    work_guard_.reset();
    
    // 关闭套接字
    boost::system::error_code ec;
    socket_.close(ec);
    
    // 等待IO线程结束
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    // 重置io_context
    io_context_.stop();
    io_context_.restart();
    
    std::cout << "UDP Multicast Server stopped" << std::endl;
}

// 发送组播消息
void UdpMulticastServer::sendMessage(const std::string& message) {
    if (!running_) {
        return;
    }
    
    // 使用strand确保线程安全
    net::dispatch(net::make_strand(io_context_), [this, message = std::string(message)]() {
        try {
            socket_.send_to(net::buffer(message), multicast_endpoint_);
        }
        catch (const std::exception& e) {
            std::cerr << "UDP Multicast send error: " << e.what() << std::endl;
        }
    });
}

// 设置消息处理器
void UdpMulticastServer::setMessageHandler(UdpMessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_handler_ = std::move(handler);
}

// 接收消息
void UdpMulticastServer::doReceive() {
    if (!running_) {
        return;
    }
    
    socket_.async_receive_from(
        net::buffer(recv_buffer_), multicast_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec && bytes_transferred > 0) {
                // 处理接收到的消息
                std::string message(recv_buffer_.data(), bytes_transferred);
                handleMessage(message, multicast_endpoint_);
            }
            else if (ec && ec != boost::asio::error::operation_aborted) {
                std::cerr << "UDP Multicast receive error: " << ec.message() << std::endl;
            }
            
            // 继续接收下一条消息
            if (running_) {
                doReceive();
            }
        });
}

// 处理接收到的消息
void UdpMulticastServer::handleMessage(const std::string& message, const boost::asio::ip::udp::endpoint& sender) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (message_handler_) {
        try {
            message_handler_(message, sender);
        }
        catch (const std::exception& e) {
            std::cerr << "UDP Message handler error: " << e.what() << std::endl;
        }
    }
}