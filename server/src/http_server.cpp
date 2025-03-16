#include "http_server.h"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

namespace cesium_server {

// HTTP会话类，处理单个HTTP请求
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(
        tcp::socket&& socket,
        std::function<http::response<http::string_body>(const http::request<http::string_body>&, const std::string&)> handler_func,
        std::function<void(tcp::socket&, http::status, const std::string&)> error_handler)
        : stream_(std::move(socket)),
          handler_func_(std::move(handler_func)),
          error_handler_(std::move(error_handler)) {
        // 设置超时
        stream_.expires_after(std::chrono::seconds(30));
    }

    void run() {
        // 开始异步读取请求
        http::async_read(stream_, buffer_, req_,
            beast::bind_front_handler(
                &HttpSession::onRead,
                shared_from_this()));
    }

private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_{8192};
    http::request<http::string_body> req_;
    std::function<http::response<http::string_body>(const http::request<http::string_body>&, const std::string&)> handler_func_;
    std::function<void(tcp::socket&, http::status, const std::string&)> error_handler_;

    void onRead(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec == http::error::end_of_stream) {
            // 客户端关闭连接
            stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
            return;
        }

        if (ec) {
            std::cerr << "Error reading request: " << ec.message() << std::endl;
            // 确保在错误时关闭连接
            beast::error_code shutdown_ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, shutdown_ec);
            if(shutdown_ec && shutdown_ec != beast::errc::not_connected) {
                std::cerr << "Error shutting down socket: " << shutdown_ec.message() << std::endl;
            }
            return;
        }

        // 处理请求
        handleRequest();
    }

    void handleRequest() {
        try {
            // 解析请求目标
            auto const& target_view = req_.target();
            std::string target_string(target_view.begin(), target_view.end());

            // 创建响应
            http::response<http::string_body> res;

            try {
                // 调用处理函数
                res = handler_func_(req_, target_string);
            } catch (const std::exception& e) {
                std::cerr << "Handler error: " << e.what() << std::endl;
                // 创建socket的副本，确保在异步操作期间socket不会被销毁
                auto self = shared_from_this();
                error_handler_(self->stream_.socket(), http::status::internal_server_error, 
                    "Internal server error processing request");
                return;
            }

            // 发送响应
            sendResponse(std::move(res));
        } catch (const std::exception& e) {
            std::cerr << "Error handling request: " << e.what() << std::endl;
            try {
                // 创建socket的副本，确保在异步操作期间socket不会被销毁
                auto self = shared_from_this();
                error_handler_(self->stream_.socket(), http::status::internal_server_error, "Internal server error");
            } catch (const std::exception& ex) {
                std::cerr << "Error sending error response: " << ex.what() << std::endl;
                // 确保在发生异常时关闭连接
                beast::error_code ec;
                stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
            }
        }
    }

    void sendResponse(http::response<http::string_body>&& res) {
        // 设置keep-alive标志
        res.keep_alive(req_.keep_alive());

        // 创建响应的共享指针，确保在异步操作期间响应对象不会被销毁
        auto res_ptr = std::make_shared<http::response<http::string_body>>(std::move(res));

        // 发送响应
        auto self = shared_from_this();
        http::async_write(stream_, *res_ptr,
            [self, res_ptr](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "Error writing response: " << ec.message() << std::endl;
                    return;
                }

                // 如果不是keep-alive，关闭连接
                if (!self->req_.keep_alive()) {
                    self->stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
                    if (ec && ec != beast::errc::not_connected) {
                        std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
                    }
                }
            });
    }
};

// HTTP Server Constructor
HttpServer::HttpServer(const std::string& address, unsigned short port, int threads)
    : address_(address), port_(port), num_threads_(threads), 
      ioc_(threads), acceptor_(net::make_strand(ioc_)), running_(false) {
    
    beast::error_code ec;

    // Open acceptor
    tcp::endpoint endpoint(net::ip::make_address(address), port);
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Error opening acceptor: " << ec.message() << std::endl;
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Error setting reuse_address: " << ec.message() << std::endl;
        return;
    }

    // Bind to port
    acceptor_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Error binding to " << address << ":" << port << ": " << ec.message() << std::endl;
        return;
    }

    // Start listening
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Error listening: " << ec.message() << std::endl;
        return;
    }

    std::cout << "HTTP server initialized on " << address << ":" << port << std::endl;
}

// Destructor
HttpServer::~HttpServer() {
    stop();
}

// Start server
void HttpServer::run() {
    if (running_) return;
    running_ = true;

    // Initialize thread pool
    thread_pool_ = std::make_unique<ThreadPool>(num_threads_);

    // Start accepting connections
    doAccept();

    // Run IO context in thread pool, not blocking main thread
    for (int i = 0; i < num_threads_; ++i) {
        thread_pool_->enqueue([this] { 
            // 直接在线程中运行io_context，不需要额外的strand
            // 因为io_context已经在构造函数中指定了线程数
            // 并且Boost.Asio会自动处理线程安全
            ioc_.run();
        });
    }

    std::cout << "HTTP server running with " << num_threads_ << " threads" << std::endl;
}

// Stop server
void HttpServer::stop() {
    if (!running_) return;
    running_ = false;

    // Stop acceptor
    beast::error_code ec;
    acceptor_.close(ec);

    // Stop IO context
    ioc_.stop();

    // Destroy thread pool
    thread_pool_.reset();

    std::cout << "HTTP server stopped" << std::endl;
}

// Register route handler
void HttpServer::registerHandler(const std::string& path, HttpRequestHandler handler) {
    handlers_[path] = handler;
    std::cout << "Registered handler for path: " << path << std::endl;
}

// Accept new connection
void HttpServer::doAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // 创建一个新的会话来处理请求
                    std::make_shared<HttpSession>(
                        std::move(socket),
                        [this](const http::request<http::string_body>& req, const std::string& target) {
                            auto handler = findHandler(target);
                            if (handler) {
                                return handler(req, target);
                            } else {
                                // 默认404响应
                                http::response<http::string_body> res(
                                    http::status::not_found, req.version());
                                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                                res.set(http::field::content_type, "text/plain");
                                
                                // 构建错误消息
                                std::stringstream ss;
                                ss << "The resource '" << target << "' was not found.";
                                res.body() = ss.str();
                                res.prepare_payload();
                                return res;
                            }
                        },
                        [this](tcp::socket& socket, http::status status, const std::string& error_message) {
                            sendErrorResponse(socket, status, error_message);
                        }
                    )->run();
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }

                // If server is still running, continue accepting connections
                if (running_) {
                    doAccept();
                }
            }));
}

// Handle request - 这个方法现在只用于兼容性，实际请求处理由HttpSession完成
void HttpServer::handleRequest(tcp::socket socket) {
    // 创建会话并启动
    std::make_shared<HttpSession>(
        std::move(socket),
        [this](const http::request<http::string_body>& req, const std::string& target) {
            auto handler = findHandler(target);
            if (handler) {
                return handler(req, target);
            } else {
                // 默认404响应
                http::response<http::string_body> res(
                    http::status::not_found, req.version());
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/plain");
                
                // 构建错误消息
                std::stringstream ss;
                ss << "The resource '" << target << "' was not found.";
                res.body() = ss.str();
                res.prepare_payload();
                return res;
            }
        },
        [this](tcp::socket& socket, http::status status, const std::string& error_message) {
            sendErrorResponse(socket, status, error_message);
        }
    )->run();
}

// Find route handler
HttpRequestHandler HttpServer::findHandler(const std::string& path) {
    // Exact match
    auto it = handlers_.find(path);
    if (it != handlers_.end()) {
        return it->second;
    }

    // Prefix match
    for (const auto& [prefix, handler] : handlers_) {
        if (path.find(prefix) == 0) {
            return handler;
        }
    }

    // 返回默认处理器而不是nullptr
    return [](const http::request<http::string_body>& req, const std::string& target) {
        http::response<http::string_body> res(http::status::not_found, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/plain");
        res.set(http::field::access_control_allow_origin, "*");
        
        std::stringstream ss;
        ss << "The resource '" << target << "' was not found.";
        res.body() = ss.str();
        res.prepare_payload();
        return res;
    };
}

// Send error response
// Send error response
void HttpServer::sendErrorResponse(tcp::socket& socket, http::status status, const std::string& error_message) {
    try {
        http::response<http::string_body> res{status, 11}; // HTTP/1.1
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        
        // Create JSON error response
        std::stringstream ss;
        ss << "{\"error\":\"" << status << "\",\"message\":\"" 
           << error_message << "\"}";
        
        res.body() = ss.str();
        res.prepare_payload();
        
        // 使用异步写入，创建socket的共享指针以确保在异步操作期间socket不会被销毁
        auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));
        auto res_ptr = std::make_shared<http::response<http::string_body>>(std::move(res));
        http::async_write(*socket_ptr, *res_ptr, 
            [res_ptr, socket_ptr](beast::error_code ec, std::size_t bytes_transferred) {
                if(ec) {
                    std::cerr << "Error sending error response: " << ec.message() << std::endl;
                }
                
                // 无论成功与否，都尝试关闭连接
                beast::error_code shutdown_ec;
                socket_ptr->shutdown(tcp::socket::shutdown_send, shutdown_ec);
                if(shutdown_ec && shutdown_ec != beast::errc::not_connected) {
                    std::cerr << "Error shutting down socket: " << shutdown_ec.message() << std::endl;
                }
            });
    } catch(const std::exception& e) {
        std::cerr << "Exception sending error response: " << e.what() << std::endl;
        // 确保在异常情况下也尝试关闭连接
        try {
            beast::error_code ec;
            socket.shutdown(tcp::socket::shutdown_both, ec);
        } catch(...) {
            // 忽略关闭连接时的异常
        }
    }
}
} // namespace cesium_server