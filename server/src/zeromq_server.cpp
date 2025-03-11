#include "../include/zeromq_server.h"
#include <chrono>
#include <iostream>
#include <sstream>

namespace cesium_server {

	// 构造函数
	ZeroMQServer::ZeroMQServer(const std::string& address, unsigned short port, Mode mode, int io_threads)
		:address_(address),
		port_(port),
		mode_(mode),
		running_(false),
		io_threads_(io_threads) {

		// 构建端点字符串
		std::ostringstream endpoint;
		endpoint << "tcp://" << address << ":" << port;
		endpoint_ = endpoint.str();

		// 初始化ZeroMQ上下文和套接字
		initialize();
	}

	// 析构函数
	ZeroMQServer::~ZeroMQServer() {
		stop();
	}

	// 初始化ZeroMQ上下文和套接字
	void ZeroMQServer::initialize() {
		try {
			// 创建ZeroMQ上下文
			context_ = std::make_unique<zmq::context_t>(io_threads_);

			// 根据通信模式创建套接字
			switch (mode_) {
			case Mode::REQ_REP:
				socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_REP);
				break;

			case Mode::PUB_SUB:
				socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_PUB);
				break;

			case Mode::PUSH_PULL:
				socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_PUSH);
				break;
			}

			// 设置套接字选项
			int linger = 0;
			socket_->set(zmq::sockopt::linger, linger);

			std::cout << "ZeroMQ server initialized with endpoint: " << endpoint_ << std::endl;
		}
		catch (const zmq::error_t& e) {
			std::cerr << "ZeroMQ initialization error: " << e.what() << std::endl;
			throw;
		}
	}

	// 启动服务器
	void ZeroMQServer::run() {
		if (running_) {
			std::cout << "ZeroMQ server already running" << std::endl;
			return;
		}

		try {
			// 绑定套接字到端点
			socket_->bind(endpoint_);

			// 设置运行标志
			running_ = true;

			// 初始化线程池
			thread_pool_ = std::make_unique<ThreadPool>(1);

			// 根据通信模式启动相应的处理任务
			switch (mode_) {
			case Mode::REQ_REP:
				thread_pool_->enqueue(&ZeroMQServer::handleReqRep, this);
				break;

			case Mode::PUB_SUB:
				thread_pool_->enqueue(&ZeroMQServer::handlePubSub, this);
				break;

			case Mode::PUSH_PULL:
				thread_pool_->enqueue(&ZeroMQServer::handlePushPull, this);
				break;
			}

			std::cout << "ZeroMQ server started in "
				<< (mode_ == Mode::REQ_REP ? "REQ-REP" :
					mode_ == Mode::PUB_SUB ? "PUB-SUB" : "PUSH-PULL")
				<< " mode" << std::endl;
		}
		catch (const zmq::error_t& e) {
			std::cerr << "ZeroMQ run error: " << e.what() << std::endl;
			running_ = false;
			throw;
		}
	}

	// 停止服务器
	void ZeroMQServer::stop() {
		if (!running_) {
			return;
		}

		// 设置停止标志
		running_ = false;

		// 销毁线程池
		thread_pool_.reset();

		try {
			// 关闭套接字
			if (socket_) {
				socket_->close();
			}

			// 清空消息队列
			std::queue<std::pair<std::string, std::string>> empty;
			std::swap(message_queue_, empty);

			std::cout << "ZeroMQ server stopped" << std::endl;
		}
		catch (const zmq::error_t& e) {
			std::cerr << "ZeroMQ stop error: " << e.what() << std::endl;
		}
	}

	// 发送消息
	bool ZeroMQServer::sendMessage(const std::string& message, const std::string& topic) {
		if (!running_ || !socket_) {
			std::cerr << "ZeroMQ server not running" << std::endl;
			return false;
		}

		try {
			// 根据通信模式发送消息
			switch (mode_) {
			case Mode::REQ_REP: {
				// 在REQ-REP模式下，只有收到请求后才能发送响应
				zmq::message_t zmq_message(message.data(), message.size());
				socket_->send(zmq_message, zmq::send_flags::none);
				break;
			}

			case Mode::PUB_SUB: {
				// 在PUB-SUB模式下，需要先发送主题，再发送消息
				std::string full_message = topic + " " + message;
				zmq::message_t zmq_message(full_message.data(), full_message.size());
				socket_->send(zmq_message, zmq::send_flags::none);
				break;
			}

			case Mode::PUSH_PULL: {
				// 在PUSH-PULL模式下，直接发送消息
				zmq::message_t zmq_message(message.data(), message.size());
				socket_->send(zmq_message, zmq::send_flags::none);
				break;
			}
			}

			return true;
		}
		catch (const zmq::error_t& e) {
			std::cerr << "ZeroMQ send error: " << e.what() << std::endl;
			return false;
		}
	}

	// 设置消息处理器
	void ZeroMQServer::setMessageHandler(ZmqMessageHandler handler) {
		message_handler_ = std::move(handler);
	}

	// 请求-响应模式处理
	void ZeroMQServer::handleReqRep() {
		while (running_) {
			try {
				// 接收请求
				zmq::message_t request;

				// 使用轮询方式接收消息，避免阻塞
				zmq::pollitem_t items[] = {
					{ socket_->handle(), 0, ZMQ_POLLIN, 0 }
				};

				zmq::poll(items, 1, std::chrono::milliseconds(100));

				if (items[0].revents & ZMQ_POLLIN) {
					auto result = socket_->recv(request, zmq::recv_flags::none);

					if (result) {
						// 将ZeroMQ消息转换为字符串
						std::string message(static_cast<char*>(request.data()), request.size());

						// 处理消息
						processMessage(message, "");
					}
				}
			}
			catch (const zmq::error_t& e) {
				if (running_) {
					std::cerr << "ZeroMQ REQ-REP error: " << e.what() << std::endl;
				}
			}
		}
	}

	// 发布-订阅模式处理
	void ZeroMQServer::handlePubSub() {
		// 在PUB-SUB模式下，服务器只发布消息，不接收消息
		// 因此这里只需要一个简单的循环来保持线程运行
		while (running_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// 推送-拉取模式处理
	void ZeroMQServer::handlePushPull() {
		// 在PUSH-PULL模式下，服务器只推送消息，不接收消息
		// 因此这里只需要一个简单的循环来保持线程运行
		while (running_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// 处理接收到的消息
	void ZeroMQServer::processMessage(const std::string& message, const std::string& topic) {
		// 如果设置了消息处理器，则调用它
		if (message_handler_) {
			try {
				message_handler_(message, topic);
			}
			catch (const std::exception& e) {
				std::cerr << "Error in ZeroMQ message handler: " << e.what() << std::endl;
			}
		}
	}

} // namespace cesium_server