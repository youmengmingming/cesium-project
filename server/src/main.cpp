#include "cesium_server_app.h"
#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>

// Global server application pointer for signal handling
cesium_server::CesiumServerApp* g_app = nullptr;

// Signal handler function
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << std::endl;
    if (g_app) {
        std::cout << "Stopping server..." << std::endl;
        g_app->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        // Create server configuration
        cesium_server::ServerConfig config;
        
        // Parse command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--http-address" && i + 1 < argc) {
                config.http_address = argv[++i];
            } else if (arg == "--http-port" && i + 1 < argc) {
                config.http_port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else if (arg == "--ws-address" && i + 1 < argc) {
                config.ws_address = argv[++i];
            } else if (arg == "--ws-port" && i + 1 < argc) {
                config.ws_port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else if (arg == "--zmq-address" && i + 1 < argc) {
                config.zmq_address = argv[++i];
            } else if (arg == "--zmq-port" && i + 1 < argc) {
                config.zmq_port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else if (arg == "--zmq-mode" && i + 1 < argc) {
                std::string mode = argv[++i];
                if (mode == "req-rep") {
                    config.zmq_mode = cesium_server::ZeroMQServer::Mode::REQ_REP;
                } else if (mode == "pub-sub") {
                    config.zmq_mode = cesium_server::ZeroMQServer::Mode::PUB_SUB;
                } else if (mode == "push-pull") {
                    config.zmq_mode = cesium_server::ZeroMQServer::Mode::PUSH_PULL;
                } else {
                    std::cerr << "Unknown ZeroMQ mode: " << mode << std::endl;
                    return 1;
                }
            } else if (arg == "--zmq-disable") {
                config.enable_zmq = false;
            } else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --http-address <address>  HTTP server address (default: 0.0.0.0)\n"
                          << "  --http-port <port>        HTTP server port (default: 3000)\n"
                          << "  --ws-address <address>    WebSocket server address (default: 0.0.0.0)\n"
                          << "  --ws-port <port>          WebSocket server port (default: 3001)\n"
                          << "  --zmq-address <address>   ZeroMQ server address (default: 0.0.0.0)\n"
                          << "  --zmq-port <port>         ZeroMQ server port (default: 5555)\n"
                          << "  --zmq-mode <mode>         ZeroMQ mode (req-rep|pub-sub|push-pull) (default: req-rep)\n"
                          << "  --zmq-disable             Disable ZeroMQ server\n"
                          << "  --help                    Show this help message\n";
                return 0;
            }
        }
        
        // Set up signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Create server application
        cesium_server::CesiumServerApp app(config);
        g_app = &app;
        
        // Start the server
        app.run();
        
        std::cout << "Server started. Press Ctrl+C to stop." << std::endl;
        std::cout << "HTTP server: http://" << config.http_address << ":" << config.http_port << std::endl;
        std::cout << "WebSocket server: ws://" << config.ws_address << ":" << config.ws_port << std::endl;
        if (config.enable_zmq) {
            std::cout << "ZeroMQ server: tcp://" << config.zmq_address << ":" << config.zmq_port;
            std::cout << " (mode: " << (config.zmq_mode == cesium_server::ZeroMQServer::Mode::REQ_REP ? "req-rep" :
                                      config.zmq_mode == cesium_server::ZeroMQServer::Mode::PUB_SUB ? "pub-sub" : "push-pull") << ")" << std::endl;
        }
        
        // Main thread waits until signal is received
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}