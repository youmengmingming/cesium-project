 #include "cesium_server_app.h"
#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>

// 全局服务器应用程序指针，用于信号处理
cesium_server::CesiumServerApp* g_app = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << std::endl;
    if (g_app) {
        std::cout << "Stopping server..." << std::endl;
        g_app->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        // 默认配置
        std::string http_address = "0.0.0.0";
        unsigned short http_port = 3000;
        std::string ws_address = "0.0.0.0";
        unsigned short ws_port = 3001;
        
        // 解析命令行参数
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--http-address" && i + 1 < argc) {
                http_address = argv[++i];
            } else if (arg == "--http-port" && i + 1 < argc) {
                http_port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else if (arg == "--ws-address" && i + 1 < argc) {
                ws_address = argv[++i];
            } else if (arg == "--ws-port" && i + 1 < argc) {
                ws_port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --http-address <address>  HTTP server address (default: 0.0.0.0)\n"
                          << "  --http-port <port>        HTTP server port (default: 3000)\n"
                          << "  --ws-address <address>    WebSocket server address (default: 0.0.0.0)\n"
                          << "  --ws-port <port>          WebSocket server port (default: 3001)\n"
                          << "  --help                    Show this help message\n";
                return 0;
            }
        }
        
        // 设置信号处理
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // 创建服务器应用程序
        cesium_server::CesiumServerApp app(http_address, http_port, ws_address, ws_port);
        g_app = &app;
        
        // 启动服务器
        app.run();
        
        std::cout << "Server started. Press Ctrl+C to stop." << std::endl;
        std::cout << "HTTP server: http://" << http_address << ":" << http_port << std::endl;
        std::cout << "WebSocket server: ws://" << ws_address << ":" << ws_port << std::endl;
        
        // 主线程等待，直到收到信号
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}