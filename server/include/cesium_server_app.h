#pragma once

#include "http_server.h"
#include "websocket_server.h"
#include "udp_multicast_server.h"
#include "zeromq_server.h"
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

namespace cesium_server {

// 坐标数据结构
struct Coordinates {
    double longitude;
    double latitude;
    double altitude;
    int64_t timestamp;
    
    // 默认构造函数
    Coordinates() : longitude(0.0), latitude(0.0), altitude(0.0), timestamp(0) {}
    
    // 带参数的构造函数
    Coordinates(double lon, double lat, double alt = 0.0) 
        : longitude(lon), latitude(lat), altitude(alt), 
          timestamp(std::chrono::system_clock::now().time_since_epoch().count()) {}
};

// 服务器配置结构
struct ServerConfig {
    // HTTP服务器配置
    std::string http_address;
    unsigned short http_port;
    int http_threads;
    
    // WebSocket服务器配置
    std::string ws_address;
    unsigned short ws_port;
    int ws_threads;
    
    // UDP组播服务器配置
    std::string udp_multicast_address;
    unsigned short udp_port;
    std::string udp_listen_address;
    size_t udp_buffer_size;
    
    // ZeroMQ服务器配置
    std::string zmq_address;
    unsigned short zmq_port;
    ZeroMQServer::Mode zmq_mode;
    int zmq_io_threads;
    bool enable_zmq;
    
    // 模拟数据配置
    bool enable_simulation;
    int simulation_interval_seconds;
    
    // 默认构造函数
    ServerConfig() 
        : http_address("127.0.0.1"), http_port(3000), http_threads(2),
          ws_address("127.0.0.1"), ws_port(3001), ws_threads(2),
          udp_multicast_address("239.255.0.1"), udp_port(5000),
          udp_listen_address("127.0.0.1"), udp_buffer_size(8192),
          zmq_address("127.0.0.1"), zmq_port(5555), 
          zmq_mode(ZeroMQServer::Mode::PUB_SUB), zmq_io_threads(1), enable_zmq(true),
          enable_simulation(true), simulation_interval_seconds(5) {}
};

// 服务器应用程序类
class CesiumServerApp {
public:
    // 使用默认配置构造
    CesiumServerApp();
    
    // 使用基本配置构造
    CesiumServerApp(const std::string& http_address, unsigned short http_port,
                   const std::string& ws_address, unsigned short ws_port);
    
    // 使用完整配置构造
    explicit CesiumServerApp(const ServerConfig& config);
    
    ~CesiumServerApp();

    // 启动服务器
    void run();

    // 停止服务器
    void stop();
    
    // 获取当前配置
    const ServerConfig& getConfig() const { return config_; }
    
    // 获取当前连接的客户端数量
    int getClientCount() const { return client_count_.load(); }
    
    // 获取最新坐标
    Coordinates getLatestCoordinates() const;
    
    // 更新坐标
    void updateCoordinates(const Coordinates& coords);

private:
    // HTTP 请求处理器
    http::response<http::string_body> handleHttpRequest(
        const http::request<http::string_body>& req, 
        const std::string& path);

    // 处理坐标请求
    http::response<http::string_body> handleCoordinatesRequest(
        const http::request<http::string_body>& req);

    // WebSocket 消息处理器
    void handleWebSocketMessage(
        const std::string& message,
        const std::shared_ptr<WebSocketSession>& session);

    // WebSocket 连接处理器
    void handleWebSocketConnection(
        const std::shared_ptr<WebSocketSession>& session,
        bool connected);
        
    // UDP 消息处理器
    void handleUdpMessage(
        const std::string& message,
        const udp::endpoint& sender);
        
    // ZeroMQ 消息处理器
    void handleZmqMessage(
        const std::string& message,
        const std::string& topic);

    // 模拟数据生成线程
    void simulationThread();
    
    // 初始化服务器
    void initialize();

    // 服务器配置
    ServerConfig config_;

    // HTTP 服务器
    std::unique_ptr<HttpServer> http_server_;

    // WebSocket 服务器
    std::unique_ptr<WebSocketServer> ws_server_;
    
    // UDP组播服务器
    std::unique_ptr<UdpMulticastServer> udp_server_;
    
    // ZeroMQ服务器
    std::unique_ptr<ZeroMQServer> zmq_server_;

    // 最新坐标
    Coordinates latest_coordinates_;
    std::mutex coordinates_mutex_;

    // 模拟数据线程
    std::thread simulation_thread_;
    std::atomic<bool> simulation_running_;

    // 客户端连接计数
    std::atomic<int> client_count_;
    
    // 客户端会话映射表 (用于存储会话特定数据)
    std::unordered_map<std::shared_ptr<WebSocketSession>, std::string> client_sessions_;
    std::mutex sessions_mutex_;
};

} // namespace cesium_server