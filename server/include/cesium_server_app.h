#pragma once

#include "http_server.h"
#include "websocket_server.h"
#include "udp_multicast_server.h"
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace cesium_server {

// 坐标数据结构
struct Coordinates {
    double longitude;
    double latitude;
};

// 服务器应用程序类
class CesiumServerApp {
public:
    CesiumServerApp(const std::string& http_address, unsigned short http_port,
                   const std::string& ws_address, unsigned short ws_port);
    ~CesiumServerApp();

    // 启动服务器
    void run();

    // 停止服务器
    void stop();

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

    // 模拟数据生成线程
    void simulationThread();

    // HTTP 服务器
    std::unique_ptr<HttpServer> http_server_;

    // WebSocket 服务器
    std::unique_ptr<WebSocketServer> ws_server_;
    
    // UDP组播服务器
    std::unique_ptr<UdpMulticastServer> udp_server_;

    // 最新坐标
    Coordinates latest_coordinates_;
    std::mutex coordinates_mutex_;

    // 模拟数据线程
    std::thread simulation_thread_;
    std::atomic<bool> simulation_running_;

    // 客户端连接计数
    std::atomic<int> client_count_;
};

} // namespace cesium_server