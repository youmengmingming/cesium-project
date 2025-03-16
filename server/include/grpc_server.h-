#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "thread_pool.h"

namespace cesium_server {

class GrpcServer {
public:
    // 构造函数
    GrpcServer(const std::string& address, unsigned short port, int threads = 1);
    
    // 析构函数
    ~GrpcServer();

    // 启动服务器
    void run();

    // 停止服务器
    void stop();

    // 获取服务器状态
    bool isRunning() const { return running_; }

    // 获取地址
    std::string getAddress() const { return address_; }

    // 获取端口
    unsigned short getPort() const { return port_; }

    // 添加服务
    template<typename ServiceType>
    void registerService(std::unique_ptr<ServiceType> service) {
        if (!running_) {
            builder_.RegisterService(service.release());
        }
    }

protected:
    // 初始化服务器
    virtual void initialize();

    // 关闭服务器
    virtual void shutdown();

private:
    // 服务器地址和端口
    std::string address_;
    unsigned short port_;
    std::string endpoint_;

    // gRPC服务器构建器和实例
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_;

    // 线程池
    std::unique_ptr<ThreadPool> thread_pool_;

    // 运行标志
    std::atomic<bool> running_{false};

    // 互斥锁
    std::mutex mutex_;

    // 服务器线程
    std::thread server_thread_;
};

} // namespace cesium_server