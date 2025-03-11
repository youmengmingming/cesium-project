#pragma once

#include <zmq.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_map>
#include "thread_pool.h"

namespace cesium_server {

	// ZeroMQ消息处理器类型
	using ZmqMessageHandler = std::function<void(const std::string&, const std::string&)>;

	// ZeroMQ服务器类
	class ZeroMQServer {
	public:
		// 通信模式枚举
		enum class Mode {
			REQ_REP,    // 请求-响应模式
			PUB_SUB,    // 发布-订阅模式
			PUSH_PULL   // 推送-拉取模式
		};

		// 构造函数
		ZeroMQServer(const std::string& address, unsigned short port, Mode mode = Mode::REQ_REP, int io_threads = 1);

		// 析构函数
		~ZeroMQServer();

		// 启动服务器
		void run();

		// 停止服务器
		void stop();

		// 发送消息
		bool sendMessage(const std::string& message, const std::string& topic = "");

		// 设置消息处理器
		void setMessageHandler(ZmqMessageHandler handler);

		// 获取服务器状态
		bool isRunning() const { return running_; }

		// 获取地址
		std::string getAddress() const { return address_; }

		// 获取端口
		unsigned short getPort() const { return port_; }

		// 获取模式
		Mode getMode() const { return mode_; }

	private:
		// 初始化ZeroMQ上下文和套接字
		void initialize();

		// 请求-响应模式处理
		void handleReqRep();

		// 发布-订阅模式处理
		void handlePubSub();

		// 推送-拉取模式处理
		void handlePushPull();

		// 处理接收到的消息
		void processMessage(const std::string& message, const std::string& topic);

		// 服务器地址和端口
		std::string address_;
		unsigned short port_;
		std::string endpoint_;

		// 通信模式
		Mode mode_;

		// ZeroMQ上下文
		std::unique_ptr<zmq::context_t> context_;

		// ZeroMQ套接字
		std::unique_ptr<zmq::socket_t> socket_;

		// 消息处理器
		ZmqMessageHandler message_handler_;

		// 工作线程
		std::thread worker_thread_;

		// 运行标志
		std::atomic<bool> running_;

		// 互斥锁
		std::mutex mutex_;

		// 消息队列
		std::queue<std::pair<std::string, std::string>> message_queue_;

		// IO线程数
		int io_threads_;

		// 线程池
		std::unique_ptr<ThreadPool> thread_pool_;
	};

} // namespace cesium_server