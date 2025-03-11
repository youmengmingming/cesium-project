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
#include <chrono>
#include <queue>
#include "thread_pool.h"

using namespace cesium_server;
namespace net = boost::asio;
using udp = boost::asio::ip::udp;

// UDP组播消息处理器类型
using UdpMessageHandler = std::function<void(const std::string&, const boost::asio::ip::udp::endpoint&)>;

// UDP组播服务器类
class UdpMulticastServer {
public:
	// 构造函数
	UdpMulticastServer(const std::string& multicast_address, unsigned short port,
		const std::string& listen_address = "0.0.0.0", size_t buffer_size = 8192);
	~UdpMulticastServer();

	// 启动服务器
	void run();

	// 停止服务器
	void stop();

	// 发送组播消息
	void sendMessage(const std::string& message);

	// 设置消息处理器
	void setMessageHandler(UdpMessageHandler handler);

	// 获取服务器状态
	bool isRunning() const { return running_; }

	// 获取组播地址
	std::string getMulticastAddress() const;

	// 获取端口
	unsigned short getPort() const;

	// 获取接收缓冲区大小
	size_t getBufferSize() const { return recv_buffer_.size(); }

private:
	// 接收消息
	void doReceive();

	// 处理接收到的消息
	void handleMessage(const std::string& message, const boost::asio::ip::udp::endpoint& sender);

	// 重新连接组播组
	bool rejoinMulticastGroup();

	// IO上下文
	net::io_context io_context_;

	// 工作守卫
	std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;

	// 套接字
	udp::socket socket_;

	// 组播地址
	udp::endpoint multicast_endpoint_;

	// 监听地址
	std::string listen_address_;

	// 发送者地址
	udp::endpoint sender_endpoint_;

	// 接收缓冲区
	std::vector<char> recv_buffer_;

	// 消息处理器
	UdpMessageHandler message_handler_;

	// 线程池
	std::unique_ptr<ThreadPool> thread_pool_;

	// 运行标志
	std::atomic<bool> running_;

	// 互斥锁
	std::mutex mutex_;

	// 错误计数器
	std::atomic<int> error_count_;

	// 上次错误时间
	std::chrono::steady_clock::time_point last_error_time_;

	// 消息队列
	std::queue<std::string> send_queue_;

	// 发送中标志
	std::atomic<bool> sending_;
};
