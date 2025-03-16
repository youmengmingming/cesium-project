#include "../include/udp_multicast_server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

// Constructor
UdpMulticastServer::UdpMulticastServer(const std::string& multicast_address, unsigned short port, const std::string& listen_address, size_t buffer_size)
	:multicast_endpoint_(net::ip::make_address(multicast_address), port),
	recv_buffer_(buffer_size),
	socket_(net::make_strand(io_context_)),
	listen_address_(listen_address),
	running_(false),
	error_count_(0),
	last_error_time_(std::chrono::steady_clock::now()),
	sending_(false) {
	try {
		// 打印初始化信息
		std::cout << "Initializing UDP Multicast Server..." << std::endl;
		std::cout << "  Multicast address: " << multicast_address << std::endl;
		std::cout << "  Listen address: " << listen_address << std::endl;
		std::cout << "  Port: " << port << std::endl;
		std::cout << "  Buffer size: " << buffer_size << " bytes" << std::endl;
		
		// 验证多播地址
		net::ip::address addr = net::ip::make_address(multicast_address);
		if (!addr.is_multicast()) {
			std::cerr << "Error: " << multicast_address << " is not a valid multicast address" << std::endl;
			std::cerr << "Multicast addresses must be in the range 224.0.0.0 to 239.255.255.255" << std::endl;
			throw std::invalid_argument("Invalid multicast address");
		}
		
		// Create UDP socket
		boost::system::error_code ec;
		socket_.open(udp::v4(), ec);
		if (ec) {
			std::cerr << "Error opening socket: " << ec.message() << std::endl;
			throw std::runtime_error("Failed to open socket: " + ec.message());
		}

		// Set socket options
		socket_.set_option(udp::socket::reuse_address(true), ec);
		if (ec) {
			std::cerr << "Error setting reuse_address option: " << ec.message() << std::endl;
			throw std::runtime_error("Failed to set reuse_address option: " + ec.message());
		}
		
		// Set receive buffer size
		socket_.set_option(boost::asio::socket_base::receive_buffer_size(buffer_size), ec);
		if (ec) {
			std::cerr << "Warning: Failed to set receive buffer size: " << ec.message() << std::endl;
			// 非致命错误，继续执行
		}
		
		// Set send buffer size
		socket_.set_option(boost::asio::socket_base::send_buffer_size(buffer_size), ec);
		if (ec) {
			std::cerr << "Warning: Failed to set send buffer size: " << ec.message() << std::endl;
			// 非致命错误，继续执行
		}

		// Bind to specified port
		try {
			udp::endpoint bind_endpoint(net::ip::make_address(listen_address), port);
			std::cout << "Binding to " << bind_endpoint.address().to_string() << ":" << bind_endpoint.port() << std::endl;
			socket_.bind(bind_endpoint, ec);
			if (ec) {
				std::cerr << "Error binding to specific address: " << ec.message() << std::endl;
				
				// 尝试使用任意地址绑定
				std::cout << "Trying to bind to any address (0.0.0.0)..." << std::endl;
				socket_.bind(udp::endpoint(net::ip::address_v4::any(), port), ec);
				if (ec) {
					std::cerr << "Error binding to any address: " << ec.message() << std::endl;
					throw std::runtime_error("Failed to bind socket: " + ec.message());
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during binding: " << e.what() << std::endl;
			throw;
		}

		// Join multicast group
		try {
			// 尝试使用特定网络接口加入多播组
			if (listen_address != "0.0.0.0") {
				std::cout << "Joining multicast group using specific interface: " << listen_address << std::endl;
				net::ip::address listen_addr = net::ip::make_address(listen_address);
				
				socket_.set_option(net::ip::multicast::join_group(
					addr.to_v4(),
					listen_addr.to_v4()), ec);
			} else {
				// 使用默认接口加入多播组
				std::cout << "Joining multicast group using default interface" << std::endl;
				socket_.set_option(net::ip::multicast::join_group(addr.to_v4()), ec);
			}
			
			if (ec) {
				std::cerr << "Error joining multicast group: " << ec.message() << std::endl;
				throw std::runtime_error("Failed to join multicast group: " + ec.message());
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Exception joining multicast group: " << e.what() << std::endl;
			throw;
		}
		
		// Set multicast TTL
		socket_.set_option(net::ip::multicast::hops(1), ec);
		if (ec) {
			std::cerr << "Warning: Failed to set multicast TTL: " << ec.message() << std::endl;
			// 非致命错误，继续执行
		}
		
		// 设置多播回环选项（允许在同一主机上接收自己发送的多播消息）
		socket_.set_option(net::ip::multicast::enable_loopback(true), ec);
		if (ec) {
			std::cerr << "Warning: Failed to set multicast loopback option: " << ec.message() << std::endl;
			// 非致命错误，继续执行
		}

		std::cout << "UDP Multicast Server successfully initialized on " << listen_address
			<< ":" << port << " (group: " << multicast_address << ")" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "UDP Multicast Server initialization error: " << e.what() << std::endl;
		// 关闭套接字（如果已打开）
		boost::system::error_code ec;
		socket_.close(ec);
		throw;
	}
}

// Destructor
UdpMulticastServer::~UdpMulticastServer() {
	stop();
}

// Get multicast address
std::string UdpMulticastServer::getMulticastAddress() const {
	return multicast_endpoint_.address().to_string();
}

// Get port
unsigned short UdpMulticastServer::getPort() const {
	return multicast_endpoint_.port();
}

// Start server
void UdpMulticastServer::run() {
	if (running_) {
		return;
	}

	running_ = true;

	// Create work guard to keep io_context running
	work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(
		io_context_.get_executor());

	// Start receiving messages
	doReceive();

	// Initialize thread pool
	thread_pool_ = std::make_unique<ThreadPool>(1);

	// Start IO context
	thread_pool_->enqueue([this]() {
		try {
			io_context_.run();
		}
		catch (const std::exception& e) {
			std::cerr << "UDP Multicast Server IO error: " << e.what() << std::endl;
		}
	});

	std::cout << "UDP Multicast Server running" << std::endl;
}

// Stop server
void UdpMulticastServer::stop() {
	if (!running_) {
		return;
	}

	running_ = false;

	// Remove work guard to allow io_context to exit
	work_guard_.reset();

	// Close socket
	boost::system::error_code ec;
	socket_.close(ec);
	if (ec) {
		std::cerr << "Error closing socket: " << ec.message() << std::endl;
	}

	// Destroy thread pool
	thread_pool_.reset();

	// Reset io_context
	io_context_.stop();
	io_context_.restart();
	
	// Clear send queue
	{
		std::lock_guard<std::mutex> lock(mutex_);
		std::queue<std::string> empty;
		std::swap(send_queue_, empty);
	}

	std::cout << "UDP Multicast Server stopped" << std::endl;
}

// Send multicast message
// Send multicast message
void UdpMulticastServer::sendMessage(const std::string& message) {
	if (!running_) {
		return;
	}

	// Add message to send queue
	{
		std::lock_guard<std::mutex> lock(mutex_);
		send_queue_.push(message);
	}
	
	// If no send operation is in progress, start one
	if (!sending_.exchange(true)) {
		// Use strand to ensure thread safety
		net::dispatch(net::make_strand(io_context_), [this]() {
			try {
				// Process next message in the send queue
				std::shared_ptr<std::string> next_message = std::make_shared<std::string>();
				{
					std::lock_guard<std::mutex> lock(mutex_);
					if (send_queue_.empty()) {
						sending_ = false;
						return;
					}
					*next_message = std::move(send_queue_.front());
					send_queue_.pop();
				}
				
				// 打印发送信息
				std::cout << "Sending to " << multicast_endpoint_.address().to_string() 
					<< ":" << multicast_endpoint_.port() << ", message size: " 
					<< next_message->size() << " bytes" << std::endl;
				
				// 检查多播地址是否有效
				if (!multicast_endpoint_.address().is_multicast()) {
					std::cerr << "Error: " << multicast_endpoint_.address().to_string() 
						<< " is not a valid multicast address" << std::endl;
					sending_ = false;
					return;
				}
				
				// Send message
				boost::system::error_code ec;
				
				// 尝试同步发送以获取立即的错误反馈
				size_t bytes_sent = socket_.send_to(net::buffer(*next_message), multicast_endpoint_, 0, ec);
				if (ec) {
					std::cerr << "UDP Multicast send error (sync): " << ec.value() << " - " << ec.message() << std::endl;
					
					// 检查是否是网络不可达错误
					if (ec == boost::asio::error::network_unreachable || 
						ec == boost::asio::error::host_unreachable || 
						ec == boost::asio::error::network_down) {
						std::cerr << "Network location not found. Please check your network connection and multicast configuration." << std::endl;
						
						// 尝试重新加入多播组
						std::cout << "Attempting to rejoin multicast group..." << std::endl;
						if (rejoinMulticastGroup()) {
							// 重试发送
							bytes_sent = socket_.send_to(net::buffer(*next_message), multicast_endpoint_, 0, ec);
							if (ec) {
								std::cerr << "UDP Multicast send error after rejoin: " << ec.message() << std::endl;
							} else {
								std::cout << "Successfully sent message after rejoin: " << bytes_sent << " bytes" << std::endl;
							}
						}
					}
				} else {
					std::cout << "Successfully sent message: " << bytes_sent << " bytes" << std::endl;
				}
				
				// 继续处理队列中的下一条消息
				sending_ = false;
				if (running_ && !send_queue_.empty()) {
					sendMessage(""); // 触发处理队列中的下一条消息
				}
			}
			catch (const std::exception& e) {
				std::cerr << "UDP Multicast send exception: " << e.what() << std::endl;
				sending_ = false;
			}
		});
	}
}

// Set message handler
void UdpMulticastServer::setMessageHandler(UdpMessageHandler handler) {
	std::lock_guard<std::mutex> lock(mutex_);
	message_handler_ = std::move(handler);
}

// Rejoin multicast group
bool UdpMulticastServer::rejoinMulticastGroup() {
	try {
		// Close and reopen socket
		boost::system::error_code ec;
		socket_.close(ec);
		if (ec) {
			std::cerr << "Error closing socket for rejoin: " << ec.message() << std::endl;
			return false;
		}
		
		// 打印网络接口信息
		std::cout << "Network interface information:" << std::endl;
		std::cout << "  Listen address: " << listen_address_ << std::endl;
		std::cout << "  Multicast address: " << multicast_endpoint_.address().to_string() << std::endl;
		std::cout << "  Port: " << multicast_endpoint_.port() << std::endl;
		
		// Reopen socket
		socket_.open(udp::v4(), ec);
		if (ec) {
			std::cerr << "Error opening socket: " << ec.message() << std::endl;
			return false;
		}
		
		// Set socket options
		socket_.set_option(udp::socket::reuse_address(true), ec);
		if (ec) {
			std::cerr << "Error setting reuse_address option: " << ec.message() << std::endl;
			return false;
		}
		
		// Set receive buffer size
		socket_.set_option(boost::asio::socket_base::receive_buffer_size(recv_buffer_.size()), ec);
		if (ec) {
			std::cerr << "Error setting receive buffer size: " << ec.message() << std::endl;
			return false;
		}
		
		// Set send buffer size
		socket_.set_option(boost::asio::socket_base::send_buffer_size(recv_buffer_.size()), ec);
		if (ec) {
			std::cerr << "Error setting send buffer size: " << ec.message() << std::endl;
			return false;
		}
		
		// Bind to specified port
		try {
			udp::endpoint bind_endpoint(net::ip::make_address(listen_address_), multicast_endpoint_.port());
			std::cout << "Binding to " << bind_endpoint.address().to_string() << ":" << bind_endpoint.port() << std::endl;
			socket_.bind(bind_endpoint, ec);
			if (ec) {
				std::cerr << "Error binding socket: " << ec.message() << std::endl;
				
				// 尝试使用任意地址绑定
				std::cout << "Trying to bind to any address (0.0.0.0)..." << std::endl;
				socket_.bind(udp::endpoint(net::ip::address_v4::any(), multicast_endpoint_.port()), ec);
				if (ec) {
					std::cerr << "Error binding to any address: " << ec.message() << std::endl;
					return false;
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during binding: " << e.what() << std::endl;
			return false;
		}
		
		// Join multicast group
		try {
			// 尝试使用特定网络接口加入多播组
			if (listen_address_ != "0.0.0.0") {
				std::cout << "Joining multicast group using specific interface: " << listen_address_ << std::endl;
				net::ip::address listen_addr = net::ip::make_address(listen_address_);
				net::ip::address multicast_addr = multicast_endpoint_.address();
				
				socket_.set_option(net::ip::multicast::join_group(
					multicast_addr.to_v4(),
					listen_addr.to_v4()), ec);
			} else {
				// 使用默认接口加入多播组
				std::cout << "Joining multicast group using default interface" << std::endl;
				socket_.set_option(net::ip::multicast::join_group(
					multicast_endpoint_.address().to_v4()), ec);
			}
			
			if (ec) {
				std::cerr << "Error joining multicast group: " << ec.message() << std::endl;
				return false;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Exception joining multicast group: " << e.what() << std::endl;
			return false;
		}
		
		// Set multicast TTL
		socket_.set_option(net::ip::multicast::hops(1), ec);
		if (ec) {
			std::cerr << "Error setting multicast TTL: " << ec.message() << std::endl;
			return false;
		}
		
		// 设置多播回环选项（允许在同一主机上接收自己发送的多播消息）
		socket_.set_option(net::ip::multicast::enable_loopback(true), ec);
		if (ec) {
			std::cerr << "Error setting multicast loopback option: " << ec.message() << std::endl;
			// 这不是致命错误，可以继续
		}
		
		std::cout << "Successfully rejoined multicast group" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Error rejoining multicast group: " << e.what() << std::endl;
		return false;
	}
}

// Receive messages
void UdpMulticastServer::doReceive() {
	if (!running_) {
		return;
	}

	socket_.async_receive_from(
		net::buffer(recv_buffer_), sender_endpoint_,
		[this](boost::system::error_code ec, std::size_t bytes_transferred) {
			if (!ec && bytes_transferred > 0) {
				// Process received message
				std::shared_ptr<std::string> message = std::make_shared<std::string>(recv_buffer_.data(), bytes_transferred);
				handleMessage(*message, sender_endpoint_);

				// Reset error counter
				error_count_ = 0;
			}
			else if (ec) {
				if (ec != boost::asio::error::operation_aborted) {
					std::cerr << "UDP Multicast receive error: " << ec.message() << std::endl;

					// Increment error count and check if reconnection is needed
					auto now = std::chrono::steady_clock::now();
					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
						now - last_error_time_).count();
					
					// If errors occur frequently, try to reconnect
					if (++error_count_ > 3 && elapsed < 60) {
						std::cout << "Too many errors, attempting to rejoin multicast group..." << std::endl;
						if (rejoinMulticastGroup()) {
							error_count_ = 0;
						}
					}
					
					last_error_time_ = now;
				}
			}

			// Continue receiving next message
			if (running_) {
				doReceive();
			}
		});
}

// Handle received message
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

// Send test data
void UdpMulticastServer::sendTestData(const std::string& test_data_type) {
	if (!running_) {
		std::cerr << "UDP Multicast Server not running, cannot send test data" << std::endl;
		return;
	}
	
	std::string test_message;
	
	// 根据测试数据类型生成不同的测试数据
	if (test_data_type == "position") {
		// 位置数据测试包
		test_message = "{\"type\":\"position\",\"data\":{\"id\":1001,\"x\":120.5,\"y\":30.2,\"z\":50.0,\"timestamp\":"+
			std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
	} else if (test_data_type == "status") {
		// 状态数据测试包
		test_message = "{\"type\":\"status\",\"data\":{\"id\":1001,\"status\":\"active\",\"battery\":85,\"timestamp\":"+
			std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
	} else if (test_data_type == "alert") {
		// 告警数据测试包
		test_message = "{\"type\":\"alert\",\"data\":{\"id\":1001,\"level\":\"warning\",\"message\":\"Low battery\",\"timestamp\":"+
			std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
	} else {
		// 默认测试包
		test_message = "{\"type\":\"test\",\"data\":{\"message\":\"This is a test message\",\"timestamp\":"+
			std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"}}"; 
	}
	
	// 发送测试数据
	std::cout << "Sending UDP multicast test data: " << test_message << std::endl;
	sendMessage(test_message);
}