 #include "../include/udp_multicast_server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

// 构造函数
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
		// 创建UDP套接字
		socket_.open(udp::v4());

		// 设置套接字选项
		socket_.set_option(udp::socket::reuse_address(true));
		
		// 设置接收缓冲区大小
		socket_.set_option(boost::asio::socket_base::receive_buffer_size(buffer_size));
		
		// 设置发送缓冲区大小
		socket_.set_option(boost::asio::socket_base::send_buffer_size(buffer_size));

		// 绑定到指定端口
		socket_.bind(udp::endpoint(net::ip::make_address(listen_address), port));

		// 加入组播组
		socket_.set_option(net::ip::multicast::join_group(
			net::ip::make_address(multicast_address)));
		
		// 设置组播TTL
		socket_.set_option(net::ip::multicast::hops(1));

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

// 获取组播地址
std::string UdpMulticastServer::getMulticastAddress() const {
	return multicast_endpoint_.address().to_string();
}

// 获取端口
unsigned short UdpMulticastServer::getPort() const {
	return multicast_endpoint_.port();
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
	if (ec) {
		std::cerr << "Error closing socket: " << ec.message() << std::endl;
	}

	// 等待IO线程结束
	if (io_thread_.joinable()) {
		io_thread_.join();
	}

	// 重置io_context
	io_context_.stop();
	io_context_.restart();
	
	// 清空发送队列
	{
		std::lock_guard<std::mutex> lock(mutex_);
		std::queue<std::string> empty;
		std::swap(send_queue_, empty);
	}

	std::cout << "UDP Multicast Server stopped" << std::endl;
}

// 发送组播消息
void UdpMulticastServer::sendMessage(const std::string& message) {
	if (!running_) {
		return;
	}

	// 将消息添加到发送队列
	{
		std::lock_guard<std::mutex> lock(mutex_);
		send_queue_.push(message);
	}
	
	// 如果没有发送操作正在进行，启动一个
	if (!sending_.exchange(true)) {
		// 使用strand确保线程安全
		net::dispatch(net::make_strand(io_context_), [this]() {
			try {
				// 处理发送队列中的下一条消息
				std::string next_message;
				{
					std::lock_guard<std::mutex> lock(mutex_);
					if (send_queue_.empty()) {
						sending_ = false;
						return;
					}
					next_message = std::move(send_queue_.front());
					send_queue_.pop();
				}
				
				// 发送消息
				socket_.async_send_to(
					net::buffer(next_message),
					multicast_endpoint_,
					[this](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
						if (ec) {
							std::cerr << "UDP Multicast send error: " << ec.message() << std::endl;
						}
						
						// 继续处理队列中的下一条消息
						sending_ = false;
						if (running_) {
							sendMessage(""); // 触发处理队列中的下一条消息
						}
					});
			}
			catch (const std::exception& e) {
				std::cerr << "UDP Multicast send error: " << e.what() << std::endl;
				sending_ = false;
			}
		});
	}
}

// 设置消息处理器
void UdpMulticastServer::setMessageHandler(UdpMessageHandler handler) {
	std::lock_guard<std::mutex> lock(mutex_);
	message_handler_ = std::move(handler);
}

// 重新连接组播组
bool UdpMulticastServer::rejoinMulticastGroup() {
	try {
		// 关闭并重新打开套接字
		boost::system::error_code ec;
		socket_.close(ec);
		if (ec) {
			std::cerr << "Error closing socket for rejoin: " << ec.message() << std::endl;
			return false;
		}
		
		// 重新打开套接字
		socket_.open(udp::v4());
		socket_.set_option(udp::socket::reuse_address(true));
		socket_.set_option(boost::asio::socket_base::receive_buffer_size(recv_buffer_.size()));
		socket_.set_option(boost::asio::socket_base::send_buffer_size(recv_buffer_.size()));
		socket_.bind(udp::endpoint(net::ip::make_address(listen_address_), multicast_endpoint_.port()));
		
		// 重新加入组播组
		socket_.set_option(net::ip::multicast::join_group(
			multicast_endpoint_.address().to_v4()));
		socket_.set_option(net::ip::multicast::hops(1));
		
		std::cout << "Successfully rejoined multicast group" << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Error rejoining multicast group: " << e.what() << std::endl;
		return false;
	}
}

// 接收消息
void UdpMulticastServer::doReceive() {
	if (!running_) {
		return;
	}

	socket_.async_receive_from(
		net::buffer(recv_buffer_), sender_endpoint_,
		[this](boost::system::error_code ec, std::size_t bytes_transferred) {
			if (!ec && bytes_transferred > 0) {
				// 处理接收到的消息
				std::string message(recv_buffer_.data(), bytes_transferred);
				handleMessage(message, sender_endpoint_);

				// 重置错误计数器
				error_count_ = 0;
			}
			else if (ec) {
				if (ec != boost::asio::error::operation_aborted) {
					std::cerr << "UDP Multicast receive error: " << ec.message() << std::endl;

					// 增加错误计数并检查是否需要重新连接
					auto now = std::chrono::steady_clock::now();
					auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
						now - last_error_time_).count();
					
					// 如果错误频繁发生，尝试重新连接
					if (++error_count_ > 3 && elapsed < 60) {
						std::cout << "Too many errors, attempting to rejoin multicast group..." << std::endl;
						if (rejoinMulticastGroup()) {
							error_count_ = 0;
						}
					}
					
					last_error_time_ = now;
				}
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