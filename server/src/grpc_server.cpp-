#include "../include/grpc_server.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include <iostream>

namespace cesium_server {

// 构造函数
GrpcServer::GrpcServer(const std::string& address, unsigned short port, int threads)
    : address_(address), port_(port), running_(false) {
    // 构建端点字符串
    std::ostringstream endpoint;
    endpoint << address << ":" << port;
    endpoint_ = endpoint.str();

    // 初始化线程池
    thread_pool_ = std::make_unique<ThreadPool>(threads);

    // 初始化服务器
    initialize();
}

// 析构函数
GrpcServer::~GrpcServer() {
    stop();
}

// 初始化服务器
void GrpcServer::initialize() {
    try {
        // 设置服务器选项
        builder_.AddListeningPort(endpoint_, grpc::InsecureServerCredentials());

        std::cout << "gRPC server initialized on " << endpoint_ << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "gRPC initialization error: " << e.what() << std::endl;
        throw;
    }
}

// 启动服务器
void GrpcServer::run() {
    if (running_) {
        std::cout << "gRPC server already running" << std::endl;
        return;
    }

    try {
        // 构建并启动服务器
        server_ = builder_.BuildAndStart();
        if (!server_) {
            throw std::runtime_error("Failed to start gRPC server");
        }

        running_ = true;

        // 在新线程中等待服务器
        server_thread_ = std::thread([this]() {
            server_->Wait();
        });

        std::cout << "gRPC server started" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "gRPC run error: " << e.what() << std::endl;
        running_ = false;
        throw;
    }
}

// 停止服务器
void GrpcServer::stop() {
    if (!running_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // 停止服务器
        if (server_) {
            server_->Shutdown();
            if (server_thread_.joinable()) {
                server_thread_.join();
            }
            server_.reset();
        }

        running_ = false;

        std::cout << "gRPC server stopped" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "gRPC stop error: " << e.what() << std::endl;
    }
}

// 关闭服务器
void GrpcServer::shutdown() {
    stop();
}

} // namespace cesium_server