#include "cesium_server_app.h"
#include <boost/json.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <memory>
#include <algorithm>

namespace cesium_server {

namespace json = boost::json;

// 默认构造函数
CesiumServerApp::CesiumServerApp()
    : CesiumServerApp(ServerConfig()) {
}

// 基本配置构造函数
CesiumServerApp::CesiumServerApp(
    const std::string& http_address, unsigned short http_port,
    const std::string& ws_address, unsigned short ws_port)
    : latest_coordinates_({0.0, 0.0}),
      simulation_running_(false),
      client_count_(0) {
    
    // 设置基本配置
    config_.http_address = http_address;
    config_.http_port = http_port;
    config_.ws_address = ws_address;
    config_.ws_port = ws_port;
    
    // 初始化服务器
    initialize();
}

// 完整配置构造函数
CesiumServerApp::CesiumServerApp(const ServerConfig& config)
    : config_(config),
      latest_coordinates_({0.0, 0.0}),
      simulation_running_(false),
      client_count_(0) {
    
    // 初始化服务器
    initialize();
}

// 初始化服务器
void CesiumServerApp::initialize() {
    try {
        // 创建 HTTP 服务器
        http_server_ = std::make_unique<HttpServer>(
            config_.http_address, config_.http_port, config_.http_threads);
        
        // 注册 HTTP 路由
        http_server_->registerHandler("/coordinates", 
            [this](const http::request<http::string_body>& req, const std::string& path) {
                return handleCoordinatesRequest(req);
            });
        
        http_server_->registerHandler("/", 
            [this](const http::request<http::string_body>& req, const std::string& path) {
                return handleHttpRequest(req, path);
            });
        
        // 创建 WebSocket 服务器
        ws_server_ = std::make_unique<WebSocketServer>(
            config_.ws_address, config_.ws_port, config_.ws_threads);
        
        // 设置 WebSocket 消息处理器
        ws_server_->setMessageHandler(
            [this](const std::string& message, const std::shared_ptr<WebSocketSession>& session) {
                handleWebSocketMessage(message, session);
            });
        
        // 设置 WebSocket 连接处理器
        ws_server_->setConnectionHandler(
            [this](const std::shared_ptr<WebSocketSession>& session, bool connected) {
                handleWebSocketConnection(session, connected);
            });
        
        // 创建 UDP 组播服务器
        udp_server_ = std::make_unique<UdpMulticastServer>(
            config_.udp_multicast_address, 
            config_.udp_port,
            config_.udp_listen_address,
            config_.udp_buffer_size);
        
        // 设置 UDP 消息处理器
        udp_server_->setMessageHandler(
            [this](const std::string& message, const udp::endpoint& sender) {
                handleUdpMessage(message, sender);
            });
        
        // 创建 ZeroMQ 服务器
        if (config_.enable_zmq) {
            zmq_server_ = std::make_unique<ZeroMQServer>(
                config_.zmq_address,
                config_.zmq_port,
                config_.zmq_mode,
                config_.zmq_io_threads);
            
            // 设置 ZeroMQ 消息处理器
            zmq_server_->setMessageHandler(
                [this](const std::string& message, const std::string& topic) {
                    handleZmqMessage(message, topic);
                });
        }
        
        std::cout << "Cesium Server Application initialized" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing Cesium Server Application: " << e.what() << std::endl;
        throw;
    }
}

// 析构函数
CesiumServerApp::~CesiumServerApp() {
    stop();
}

// 启动服务器
void CesiumServerApp::run() {
    try {
        // 启动 HTTP 服务器
        if (http_server_) {
            http_server_->run();
        }
        
        // 启动 WebSocket 服务器
        if (ws_server_) {
            ws_server_->run();
        }
        
        // 启动 UDP 组播服务器
        if (udp_server_) {
            udp_server_->run();
        }
        
        // 启动 ZeroMQ 服务器
        if (zmq_server_ && config_.enable_zmq) {
            zmq_server_->run();
        }
        
        // 启动模拟数据线程
        if (config_.enable_simulation) {
            simulation_running_ = true;
            simulation_thread_ = std::thread(&CesiumServerApp::simulationThread, this);
        }
        
        std::cout << "Cesium Server Application running" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error starting Cesium Server Application: " << e.what() << std::endl;
        stop();
        throw;
    }
}

// 停止服务器
void CesiumServerApp::stop() {
    // 停止模拟数据线程
    simulation_running_ = false;
    if (simulation_thread_.joinable()) {
        simulation_thread_.join();
    }
    
    // 停止 ZeroMQ 服务器
    if (zmq_server_) {
        try {
            zmq_server_->stop();
        } catch (const std::exception& e) {
            std::cerr << "Error stopping ZeroMQ server: " << e.what() << std::endl;
        }
    }
    
    // 停止 UDP 组播服务器
    if (udp_server_) {
        try {
            udp_server_->stop();
        } catch (const std::exception& e) {
            std::cerr << "Error stopping UDP server: " << e.what() << std::endl;
        }
    }
    
    // 停止 WebSocket 服务器
    if (ws_server_) {
        try {
            ws_server_->stop();
        } catch (const std::exception& e) {
            std::cerr << "Error stopping WebSocket server: " << e.what() << std::endl;
        }
    }
    
    // 停止 HTTP 服务器
    if (http_server_) {
        try {
            http_server_->stop();
        } catch (const std::exception& e) {
            std::cerr << "Error stopping HTTP server: " << e.what() << std::endl;
        }
    }
    
    // 清空客户端会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        client_sessions_.clear();
    }
    
    std::cout << "Cesium Server Application stopped" << std::endl;
}

// 获取最新坐标
Coordinates CesiumServerApp::getLatestCoordinates() const {
    std::lock_guard<std::mutex> lock(coordinates_mutex_);
    return latest_coordinates_;
}

// 更新坐标
void CesiumServerApp::updateCoordinates(const Coordinates& coords) {
    {
        std::lock_guard<std::mutex> lock(coordinates_mutex_);
        latest_coordinates_ = coords;
    }
    
    // 创建广播消息
    json::object broadcast_obj;
    broadcast_obj["type"] = "coordinates_update";
    broadcast_obj["longitude"] = coords.longitude;
    broadcast_obj["latitude"] = coords.latitude;
    broadcast_obj["altitude"] = coords.altitude;
    broadcast_obj["timestamp"] = coords.timestamp;
    
    // 广播给所有WebSocket客户端
    try {
        if (ws_server_ && client_count_.load() > 0) {
            ws_server_->broadcast(json::serialize(broadcast_obj));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error broadcasting coordinates update: " << e.what() << std::endl;
    }
}

// 处理 ZeroMQ 消息
void CesiumServerApp::handleZmqMessage(const std::string& message, const std::string& topic) {
    try {
        // 解析消息
        auto msg = json::parse(message);
        
        // 根据消息类型处理
        if (msg.is_object()) {
            auto obj = msg.as_object();
            if (obj.contains("type")) {
                std::string type = obj["type"].as_string().c_str();
                
                if (type == "get_coordinates") {
                    // 获取坐标请求
                    auto coords = getLatestCoordinates();
                    
                    // 创建响应
                    json::object response;
                    response["type"] = "coordinates";
                    response["longitude"] = coords.longitude;
                    response["latitude"] = coords.latitude;
                    response["altitude"] = coords.altitude;
                    response["timestamp"] = coords.timestamp;
                    
                    // 发送响应
                    if (zmq_server_) {
                        zmq_server_->sendMessage(json::serialize(response), topic);
                    }
                }
                else if (type == "update_coordinates") {
                    // 更新坐标请求
                    if (obj.contains("longitude") && obj.contains("latitude")) {
                        double longitude = obj["longitude"].to_number<double>();
                        double latitude = obj["latitude"].to_number<double>();
                        double altitude = obj.contains("altitude") ? 
                            obj["altitude"].to_number<double>() : 0.0;
                        
                        // 更新坐标
                        updateCoordinates({longitude, latitude, altitude});
                        
                        // 创建响应
                        json::object response;
                        response["type"] = "coordinates_updated";
                        response["status"] = "ok";
                        
                        // 发送响应
                        if (zmq_server_) {
                            zmq_server_->sendMessage(json::serialize(response), topic);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling ZeroMQ message: " << e.what() << std::endl;
    }
}

// HTTP 请求处理器
http::response<http::string_body> CesiumServerApp::handleHttpRequest(
    const http::request<http::string_body>& req, 
    const std::string& path) {
    
    // 创建响应
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    
    // 处理 OPTIONS 请求（CORS 预检）
    if (req.method() == http::verb::options) {
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type");
        res.body() = "";
        res.prepare_payload();
        return res;
    }
    
    // 处理根路径请求
    if (path == "/") {
        json::object response;
        response["status"] = "ok";
        response["message"] = "Cesium Server is running";
        response["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        response["clients"] = client_count_.load();
        
        // 添加服务器配置信息
        json::object config;
        config["http_port"] = config_.http_port;
        config["ws_port"] = config_.ws_port;
        config["udp_port"] = config_.udp_port;
        config["udp_multicast_address"] = config_.udp_multicast_address;
        response["config"] = config;
        
        res.body() = json::serialize(response);
        res.prepare_payload();
        return res;
    }
    
    // 默认 404 响应
    res.result(http::status::not_found);
    res.body() = json::serialize(json::object{
        {"error", "Not found"},
        {"path", path}
    });
    res.prepare_payload();
    return res;
}

// 处理坐标请求
http::response<http::string_body> CesiumServerApp::handleCoordinatesRequest(
    const http::request<http::string_body>& req) {
    
    // 创建响应
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    
    // 处理 OPTIONS 请求（CORS 预检）
    if (req.method() == http::verb::options) {
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type");
        res.body() = "";
        res.prepare_payload();
        return res;
    }
    
    // 处理 POST 请求（接收坐标）
    if (req.method() == http::verb::post) {
        try {
            // 解析 JSON
            auto json_value = json::parse(req.body());
            auto obj = json_value.as_object();
            
            // 提取坐标
            double longitude = obj["longitude"].as_double();
            double latitude = obj["latitude"].as_double();
            double altitude = 0.0;
            
            // 尝试获取高度（可选）
            if (obj.contains("altitude")) {
                altitude = obj["altitude"].as_double();
            }
            
            // 创建坐标对象
            Coordinates coords(longitude, latitude, altitude);
            
            // 更新最新坐标并广播
            updateCoordinates(coords);
            
            std::cout << "Received coordinates: " << longitude << ", " << latitude << ", " << altitude << std::endl;
            
            // 返回成功响应
            res.body() = json::serialize(json::object{
                {"status", "ok"},
                {"message", "Coordinates received"}
            });
            res.prepare_payload();
            return res;
        } catch (const std::exception& e) {
            // 返回错误响应
            res.result(http::status::bad_request);
            res.body() = json::serialize(json::object{
                {"error", "Invalid JSON"},
                {"message", e.what()}
            });
            res.prepare_payload();
            return res;
        }
    }
    
    // 处理 GET 请求（获取坐标）
    if (req.method() == http::verb::get) {
        Coordinates coords = getLatestCoordinates();
        
        json::object response;
        response["longitude"] = coords.longitude;
        response["latitude"] = coords.latitude;
        response["altitude"] = coords.altitude;
        response["timestamp"] = coords.timestamp;
        
        res.body() = json::serialize(response);
        res.prepare_payload();
        return res;
    }
    
    // 不支持的方法
    res.result(http::status::method_not_allowed);
    res.body() = json::serialize(json::object{
        {"error", "Method not allowed"}
    });
    res.prepare_payload();
    return res;
}

// WebSocket 消息处理器
void CesiumServerApp::handleWebSocketMessage(
    const std::string& message,
    const std::shared_ptr<WebSocketSession>& session) {
    
    try {
        // 解析 JSON
        auto json_value = json::parse(message);
        auto obj = json_value.as_object();
        
        // 处理不同类型的消息
        std::string type = obj["type"].as_string().c_str();
        
        if (type == "ping") {
            // 处理 ping 消息
            json::object response;
            response["type"] = "pong";
            response["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            
            // 发送响应
            try {
                session->send(json::serialize(response));
            } catch (const std::exception& e) {
                std::cerr << "Error sending pong response: " << e.what() << std::endl;
            }
        } else if (type == "get_coordinates") {
            // 处理获取坐标请求
            std::lock_guard<std::mutex> lock(coordinates_mutex_);
            
            json::object response;
            response["type"] = "coordinates";
            response["longitude"] = latest_coordinates_.longitude;
            response["latitude"] = latest_coordinates_.latitude;
            response["altitude"] = latest_coordinates_.altitude;
            response["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            
            // 发送响应
            try {
                session->send(json::serialize(response));
            } catch (const std::exception& e) {
                std::cerr << "Error sending coordinates response: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
    }
}

// WebSocket 连接处理器
void CesiumServerApp::handleWebSocketConnection(
    const std::shared_ptr<WebSocketSession>& session,
    bool connected) {
    
    if (connected) {
        // 客户端连接
        client_count_++;
        std::cout << "WebSocket client connected. Total clients: " << client_count_.load() << std::endl;
        
        // 发送欢迎消息
        json::object welcome;
        welcome["type"] = "welcome";
        welcome["message"] = "Welcome to Cesium Server";
        welcome["clients"] = client_count_.load();
        welcome["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        // 发送响应
        try {
            session->send(json::serialize(welcome));
        } catch (const std::exception& e) {
            std::cerr << "Error sending welcome message: " << e.what() << std::endl;
        }
    } else {
        // 客户端断开连接
        client_count_--;
        std::cout << "WebSocket client disconnected. Total clients: " << client_count_.load() << std::endl;
    }
}

// UDP 消息处理器
void CesiumServerApp::handleUdpMessage(
    const std::string& message,
    const udp::endpoint& sender) {
    try {
        auto json_value = json::parse(message);
        auto obj = json_value.as_object();
        
        // 处理不同类型的消息
        std::string type = obj["type"].as_string().c_str();
        
        if (type == "coordinates") {
            double longitude = obj["longitude"].as_double();
            double latitude = obj["latitude"].as_double();
            
            // 更新最新坐标
            {
                std::lock_guard<std::mutex> lock(coordinates_mutex_);
                latest_coordinates_.longitude = longitude;
                latest_coordinates_.latitude = latitude;
            }
            
            std::cout << "Received UDP coordinates: " << longitude << ", " << latitude << std::endl;
            
            // 广播坐标给所有 WebSocket 客户端
            json::object broadcast_obj;
            broadcast_obj["type"] = "coordinates_update";
            broadcast_obj["longitude"] = longitude;
            broadcast_obj["latitude"] = latitude;
            broadcast_obj["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            broadcast_obj["source"] = "udp";
            
            ws_server_->broadcast(json::serialize(broadcast_obj));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing UDP message: " << e.what() << std::endl;
    }
}

// 模拟数据生成线程
void CesiumServerApp::simulationThread() {
    std::cout << "Simulation thread started" << std::endl;
    
    // 随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> lon_dist(-180.0, 180.0);
    std::uniform_real_distribution<> lat_dist(-90.0, 90.0);
    std::uniform_real_distribution<> time_dist(1.0, 5.0);
    
    while (simulation_running_) {
        try {
            // 生成随机坐标
            double longitude = lon_dist(gen);
            double latitude = lat_dist(gen);
            
            // 创建模拟数据消息
            json::object sim_data;
            sim_data["type"] = "simulation_data";
            sim_data["longitude"] = longitude;
            sim_data["latitude"] = latitude;
            sim_data["altitude"] = 1000.0 + 500.0 * std::sin(longitude * 0.1);
            sim_data["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            
            // 广播给所有客户端
            if (client_count_.load() > 0) {
                try {
                    ws_server_->broadcast(json::serialize(sim_data));
                } catch (const std::exception& e) {
                    std::cerr << "Error broadcasting simulation data: " << e.what() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in simulation thread: " << e.what() << std::endl;
        }
        
        // 随机等待时间
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    std::cout << "Simulation thread stopped" << std::endl;
}

} // namespace cesium_server