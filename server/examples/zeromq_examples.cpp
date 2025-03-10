#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// 请求-响应模式客户端示例
void reqRepClient() {
    std::cout << "=== 请求-响应模式示例 ===" << std::endl;
    
    // 创建ZeroMQ上下文
    zmq::context_t context(1);
    
    // 创建请求套接字
    zmq::socket_t socket(context, ZMQ_REQ);
    
    // 连接到服务器
    std::cout << "连接到服务器..." << std::endl;
    socket.connect("tcp://localhost:5555");
    
    // 发送请求
    for (int i = 0; i < 3; ++i) {
        // 创建请求消息
        std::string request = "{\"type\":\"get_coordinates\"}";
        
        // 发送请求
        std::cout << "发送请求: " << request << std::endl;
        socket.send(zmq::buffer(request), zmq::send_flags::none);
        
        // 接收响应
        zmq::message_t reply;
        auto result = socket.recv(reply, zmq::recv_flags::none);
        
        if (result) {
            std::string response(static_cast<char*>(reply.data()), reply.size());
            std::cout << "接收响应: " << response << std::endl;
        }
        
        // 等待一秒
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// 发布-订阅模式客户端示例
void pubSubClient() {
    std::cout << "\n=== 发布-订阅模式示例 ===" << std::endl;
    
    // 创建ZeroMQ上下文
    zmq::context_t context(1);
    
    // 创建订阅套接字
    zmq::socket_t socket(context, ZMQ_SUB);
    
    // 连接到服务器
    std::cout << "连接到服务器..." << std::endl;
    socket.connect("tcp://localhost:5556");
    
    // 设置订阅过滤器，空字符串表示接收所有消息
    socket.set(zmq::sockopt::subscribe, "");
    
    // 接收消息
    std::cout << "等待消息..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        zmq::message_t message;
        auto result = socket.recv(message, zmq::recv_flags::none);
        
        if (result) {
            std::string data(static_cast<char*>(message.data()), message.size());
            std::cout << "接收消息: " << data << std::endl;
        }
    }
}

// 推送-拉取模式客户端示例
void pushPullClient() {
    std::cout << "\n=== 推送-拉取模式示例 ===" << std::endl;
    
    // 创建ZeroMQ上下文
    zmq::context_t context(1);
    
    // 创建拉取套接字
    zmq::socket_t socket(context, ZMQ_PULL);
    
    // 连接到服务器
    std::cout << "连接到服务器..." << std::endl;
    socket.connect("tcp://localhost:5557");
    
    // 接收消息
    std::cout << "等待消息..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        zmq::message_t message;
        auto result = socket.recv(message, zmq::recv_flags::none);
        
        if (result) {
            std::string data(static_cast<char*>(message.data()), message.size());
            std::cout << "接收消息: " << data << std::endl;
        }
    }
}

// 服务器端示例：请求-响应模式
void reqRepServer() {
    // 创建ZeroMQ上下文
    zmq::context_t context(1);
    
    // 创建响应套接字
    zmq::socket_t socket(context, ZMQ_REP);
    
    // 绑定地址
    socket.bind("tcp://*:5555");
    
    std::cout << "REQ-REP服务器启动，等待请求..." << std::endl;
    
    while (true) {
        // 接收请求
        zmq::message_t request;
        auto result = socket.recv(request, zmq::recv_flags::none);
        
        if (result) {
            std::string message(static_cast<char*>(request.data()), request.size());
            std::cout << "接收请求: " << message << std::endl;
            
            // 处理请求
            std::string response = "{\"type\":\"coordinates\",\"longitude\":116.3912,\"latitude\":39.9073,\"altitude\":0.0,\"timestamp\":1646123456789}";
            
            // 发送响应
            std::cout << "发送响应: " << response << std::endl;
            socket.send(zmq::buffer(response), zmq::send_flags::none);
        }
    }
}

// 服务器端示例：发布-订阅模式
void pubSubServer() {
    // 创建ZeroMQ上下文
    zmq::context_t context(1);
    
    // 创建发布套接字
    zmq::socket_t socket(context, ZMQ_PUB);
    
    // 绑定地址
    socket.bind("tcp://*:5556");
    
    std::cout << "PUB-SUB服务器启动，开始发布消息..." << std::endl;
    
    int count = 0;
    while (true) {
        // 创建消息
        std::string topic = "coordinates";
        std::string message = "{\"type\":\"coordinates_update\",\"longitude\":" + std::to_string(116.3912 + count * 0.001) + 
                         ",\"latitude\":" + std::to_string(39.9073 + count * 0.001) + ",\"timestamp\":1646123456789}";
        
        // 发送消息
        std::string full_message = topic + " " + message;
        std::cout << "发布消息: " << full_message << std::endl;
        socket.send(zmq::buffer(full_message), zmq::send_flags::none);
        
        // 等待一秒
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
    }
}

// 服务器端示例：推送-拉取模式
void pushPullServer() {
    // 创建ZeroMQ上下文
    zmq::context_t context(1);
    
    // 创建推送套接字
    zmq::socket_t socket(context, ZMQ_PUSH);
    
    // 绑定地址
    socket.bind("tcp://*:5557");
    
    std::cout << "PUSH-PULL服务器启动，开始推送消息..." << std::endl;
    
    int count = 0;
    while (true) {
        // 创建消息
        std::string message = "{\"type\":\"simulation_data\",\"longitude\":" + std::to_string(116.3912 + count * 0.001) + 
                         ",\"latitude\":" + std::to_string(39.9073 + count * 0.001) + ",\"altitude\":" + std::to_string(1000 + count) + ",\"timestamp\":1646123456789}";
        
        // 发送消息
        std::cout << "推送消息: " << message << std::endl;
        socket.send(zmq::buffer(message), zmq::send_flags::none);
        
        // 等待一秒
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
    }
}

// 主函数
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " [client|server] [req-rep|pub-sub|push-pull]" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    std::string pattern = argc > 2 ? argv[2] : "req-rep";
    
    try {
        if (mode == "client") {
            if (pattern == "req-rep") {
                reqRepClient();
            } else if (pattern == "pub-sub") {
                pubSubClient();
            } else if (pattern == "push-pull") {
                pushPullClient();
            } else {
                std::cout << "未知的通信模式: " << pattern << std::endl;
                return 1;
            }
        } else if (mode == "server") {
            if (pattern == "req-rep") {
                reqRepServer();
            } else if (pattern == "pub-sub") {
                pubSubServer();
            } else if (pattern == "push-pull") {
                pushPullServer();
            } else {
                std::cout << "未知的通信模式: " << pattern << std::endl;
                return 1;
            }
        } else {
            std::cout << "未知的运行模式: " << mode << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}