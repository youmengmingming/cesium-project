#include "../include/zeromq_server.h"
#include <chrono>
#include <iostream>
#include <sstream>

namespace cesium_server {

	// Constructor
	ZeroMQServer::ZeroMQServer(const std::string& address, unsigned short port, Mode mode, int io_threads)
		:address_(address),
		port_(port),
		mode_(mode),
		running_(false),
		io_threads_(io_threads) {

		// Build endpoint string
		std::ostringstream endpoint;
		endpoint << "tcp://" << address << ":" << port;
		endpoint_ = endpoint.str();

		// Initialize ZeroMQ context and socket
		initialize();
	}

	// Destructor
	ZeroMQServer::~ZeroMQServer() {
		stop();
	}

	// Initialize ZeroMQ context and socket
	void ZeroMQServer::initialize() {
		try {
			// Create ZeroMQ context
			context_ = std::make_unique<zmq::context_t>(io_threads_);

			// Create socket based on communication mode
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

			// Set socket options
			int linger = 0;
			socket_->set(zmq::sockopt::linger, linger);

			std::cout << "ZeroMQ server initialized with endpoint: " << endpoint_ << std::endl;
		}
		catch (const zmq::error_t& e) {
			std::cerr << "ZeroMQ initialization error: " << e.what() << std::endl;
			throw;
		}
	}

	// Start server
	void ZeroMQServer::run() {
		if (running_) {
			std::cout << "ZeroMQ server already running" << std::endl;
			return;
		}

		try {
			// Bind socket to endpoint
			socket_->bind(endpoint_);

			// Set running flag
			running_ = true;

			// Initialize thread pool
			thread_pool_ = std::make_unique<ThreadPool>(1);

			// Start appropriate handler based on communication mode
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

	// Stop server
	void ZeroMQServer::stop() {
		if (!running_) {
			return;
		}

		// Set stop flag
		running_ = false;

		// Destroy thread pool
		thread_pool_.reset();

		try {
			// Close socket
			if (socket_) {
				socket_->close();
			}

			// Clear message queue
			std::queue<std::pair<std::string, std::string>> empty;
			std::swap(message_queue_, empty);

			std::cout << "ZeroMQ server stopped" << std::endl;
		}
		catch (const zmq::error_t& e) {
			std::cerr << "ZeroMQ stop error: " << e.what() << std::endl;
		}
	}

	// Send message
	bool ZeroMQServer::sendMessage(const std::string& message, const std::string& topic) {
		if (!running_ || !socket_) {
			std::cerr << "ZeroMQ server not running" << std::endl;
			return false;
		}

		try {
			// Send message based on communication mode
			switch (mode_) {
			case Mode::REQ_REP: {
				// In REQ-REP mode, can only send response after receiving request
				zmq::message_t zmq_message(message.data(), message.size());
				socket_->send(zmq_message, zmq::send_flags::none);
				break;
			}

			case Mode::PUB_SUB: {
				// In PUB-SUB mode, need to send topic first, then message
				std::string full_message = topic + " " + message;
				zmq::message_t zmq_message(full_message.data(), full_message.size());
				socket_->send(zmq_message, zmq::send_flags::none);
				break;
			}

			case Mode::PUSH_PULL: {
				// In PUSH-PULL mode, send message directly
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

	// Set message handler
	void ZeroMQServer::setMessageHandler(ZmqMessageHandler handler) {
		message_handler_ = std::move(handler);
	}

	// Request-response mode handler
	void ZeroMQServer::handleReqRep() {
		while (running_) {
			try {
				// Receive request
				zmq::message_t request;

				// Use polling to receive messages, avoid blocking
				zmq::pollitem_t items[] = {
					{ socket_->handle(), 0, ZMQ_POLLIN, 0 }
				};

				zmq::poll(items, 1, std::chrono::milliseconds(100));

				if (items[0].revents & ZMQ_POLLIN) {
					auto result = socket_->recv(request, zmq::recv_flags::none);

					if (result) {
						// Convert ZeroMQ message to string
						std::string message(static_cast<char*>(request.data()), request.size());

						// Process message
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

	// Publish-subscribe mode handler
	void ZeroMQServer::handlePubSub() {
		// In PUB-SUB mode, server only publishes messages, doesn't receive them
		// So we just need a simple loop to keep the thread running
		while (running_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// Push-pull mode handler
	void ZeroMQServer::handlePushPull() {
		// In PUSH-PULL mode, server only pushes messages, doesn't receive them
		// So we just need a simple loop to keep the thread running
		while (running_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// Process received message
	void ZeroMQServer::processMessage(const std::string& message, const std::string& topic) {
		// If message handler is set, call it
		if (message_handler_) {
			try {
				message_handler_(message, topic);
			}
			catch (const std::exception& e) {
				std::cerr << "Error in ZeroMQ message handler: " << e.what() << std::endl;
			}
		}
	}

	// Publish test data
	bool ZeroMQServer::publishTestData(const std::string& test_data_type, const std::string& topic) {
		if (!running_ || mode_ != Mode::PUB_SUB) {
			std::cerr << "ZeroMQ server not running or not in PUB-SUB mode, cannot publish test data" << std::endl;
			return false;
		}
		
		std::string test_message;
		
		// 根据测试数据类型生成不同的测试数据
		if (test_data_type == "position") {
			// 位置数据测试包
			test_message = "{\"type\":\"position\",\"data\":{\"id\":2001,\"x\":120.5,\"y\":30.2,\"z\":50.0,\"timestamp\":"+
				std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
		} else if (test_data_type == "status") {
			// 状态数据测试包
			test_message = "{\"type\":\"status\",\"data\":{\"id\":2001,\"status\":\"active\",\"battery\":85,\"timestamp\":"+
				std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
		} else if (test_data_type == "alert") {
			// 告警数据测试包
			test_message = "{\"type\":\"alert\",\"data\":{\"id\":2001,\"level\":\"warning\",\"message\":\"System overheating\",\"timestamp\":"+
				std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
		} else {
			// 默认测试包
			test_message = "{\"type\":\"test\",\"data\":{\"message\":\"This is a ZeroMQ test message\",\"timestamp\":"+
				std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
		}
		
		// 发布测试数据
		std::cout << "Publishing ZeroMQ test data to topic '" << topic << "': " << test_message << std::endl;
		return sendMessage(test_message, topic);
	}
} // namespace cesium_server