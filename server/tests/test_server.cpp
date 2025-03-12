#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <zmq.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using udp = boost::asio::ip::udp;

class ServerTest {
public:
    ServerTest(const std::string& host, unsigned short http_port, unsigned short ws_port)
        : host_(host), http_port_(http_port), ws_port_(ws_port),
          ioc_(), resolver_(net::make_strand(ioc_)), zmq_context_(1) {}

    // 测试HTTP服务器
    bool testHttpServer() {
        try {
            std::cout << "测试HTTP服务器..." << std::endl;

            // 测试GET /coordinates
            auto getResult = testHttpGet("/coordinates");
            std::cout << "GET /coordinates 响应: " << getResult << std::endl;

            // 测试POST /coordinates
            std::string postData = "{\"longitude\": 116.3912, \"latitude\": 39.9073}";
            auto postResult = testHttpPost("/coordinates", postData);
            std::cout << "POST /coordinates 响应: " << postResult << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cerr << "HTTP测试失败: " << e.what() << std::endl;
            return false;
        }
    }

    // 测试WebSocket服务器
    bool testWebSocketServer() {
        try {
            std::cout << "测试WebSocket服务器..." << std::endl;

            // 创建WebSocket连接
            tcp::socket socket{ioc_};
            auto const results = resolver_.resolve(host_, std::to_string(ws_port_));
            net::connect(socket, results.begin(), results.end());

            websocket::stream<tcp::socket> ws{std::move(socket)};
            ws.handshake(host_, "/");

            // 发送ping消息
            std::string ping_msg = "{\"type\": \"ping\"}";
            ws.write(net::buffer(ping_msg));

            // 接收pong响应
            beast::flat_buffer buffer;
            ws.read(buffer);
            std::cout << "WebSocket响应: " << beast::make_printable(buffer.data()) << std::endl;

            // 请求坐标数据
            std::string coords_msg = "{\"type\": \"get_coordinates\"}";
            ws.write(net::buffer(coords_msg));

            // 接收坐标响应
            buffer.consume(buffer.size());
            ws.read(buffer);
            std::cout << "坐标数据响应: " << beast::make_printable(buffer.data()) << std::endl;

            // 正常关闭连接
            ws.close(websocket::close_code::normal);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "WebSocket测试失败: " << e.what() << std::endl;
            return false;
        }
    }

    // 测试ZMQ请求-响应模式
    bool testZmqReqRep() {
        try {
            std::cout << "测试ZMQ请求-响应模式..." << std::endl;

            // 创建请求套接字
            zmq::socket_t socket(zmq_context_, ZMQ_REQ);
            socket.connect("tcp://localhost:5555");

            // 发送请求
            std::string request = "{\"type\":\"get_coordinates\"}";
            std::cout << "发送请求: " << request << std::endl;
            socket.send(zmq::buffer(request), zmq::send_flags::none);

            // 接收响应
            zmq::message_t reply;
            auto result = socket.recv(reply, zmq::recv_flags::none);

            if (result) {
                std::string response(static_cast<char*>(reply.data()), reply.size());
                std::cout << "接收响应: " << response << std::endl;
                return true;
            }

            return false;
        } catch (const std::exception& e) {
            std::cerr << "ZMQ请求-响应测试失败: " << e.what() << std::endl;
            return false;
        }
    }

    // 测试ZMQ发布-订阅模式
    bool testZmqPubSub() {
        try {
            std::cout << "测试ZMQ发布-订阅模式..." << std::endl;

            // 创建订阅套接字
            zmq::socket_t socket(zmq_context_, ZMQ_SUB);
            socket.connect("tcp://localhost:5556");
            socket.set(zmq::sockopt::subscribe, "");

            // 接收消息
            std::cout << "等待消息..." << std::endl;
            zmq::message_t message;
            auto result = socket.recv(message, zmq::recv_flags::none);

            if (result) {
                std::string data(static_cast<char*>(message.data()), message.size());
                std::cout << "接收消息: " << data << std::endl;
                return true;
            }

            return false;
        } catch (const std::exception& e) {
            std::cerr << "ZMQ发布-订阅测试失败: " << e.what() << std::endl;
            return false;
        }
    }

    // 测试UDP组播
    bool testUdpMulticast() {
        try {
            std::cout << "测试UDP组播..." << std::endl;

            // 创建UDP socket
            udp::socket socket(ioc_);
            socket.open(udp::v4());

            // 加入组播组
            socket.set_option(udp::socket::reuse_address(true));
            socket.bind(udp::endpoint(udp::v4(), 5000));

            net::ip::address multicast_address = net::ip::make_address("239.255.0.1");
            socket.set_option(net::ip::multicast::join_group(multicast_address));

            // 接收数据
            std::array<char, 1024> recv_buf;
            udp::endpoint sender_endpoint;

            // 设置接收超时
            socket.async_receive_from(
                net::buffer(recv_buf), sender_endpoint,
                [&](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        std::cout << "接收到UDP组播数据: " 
                                << std::string(recv_buf.data(), length) << std::endl;
                    }
                });

            // 运行IO服务
            ioc_.run_for(std::chrono::seconds(5));

            return true;
        } catch (const std::exception& e) {
            std::cerr << "UDP组播测试失败: " << e.what() << std::endl;
            return false;
        }
    }

private:
    // HTTP GET请求
    std::string testHttpGet(const std::string& target) {
        tcp::socket socket{ioc_};
        auto const results = resolver_.resolve(host_, std::to_string(http_port_));
        net::connect(socket, results.begin(), results.end());

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(socket, buffer, res);

        return res.body();
    }

    // HTTP POST请求
    std::string testHttpPost(const std::string& target, const std::string& data) {
        tcp::socket socket{ioc_};
        auto const results = resolver_.resolve(host_, std::to_string(http_port_));
        net::connect(socket, results.begin(), results.end());

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = data;
        req.prepare_payload();

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(socket, buffer, res);

        return res.body();
    }

    std::string host_;
    unsigned short http_port_;
    unsigned short ws_port_;
    net::io_context ioc_;
    tcp::resolver resolver_;
    zmq::context_t zmq_context_;
};

int main() {
    try {
        ServerTest test("localhost", 3000, 3001);

        std::cout << "开始服务器测试..." << std::endl;

        // 测试HTTP服务器
        if (!test.testHttpServer()) {
            std::cerr << "HTTP服务器测试失败" << std::endl;
            return 1;
        }

        // 测试WebSocket服务器
        if (!test.testWebSocketServer()) {
            std::cerr << "WebSocket服务器测试失败" << std::endl;
            return 1;
        }

        // 测试ZMQ请求-响应模式
        if (!test.testZmqReqRep()) {
            std::cerr << "ZMQ请求-响应测试失败" << std::endl;
            return 1;
        }

        // 测试ZMQ发布-订阅模式
        if (!test.testZmqPubSub()) {
            std::cerr << "ZMQ发布-订阅测试失败" << std::endl;
            return 1;
        }

        // 测试UDP组播
        if (!test.testUdpMulticast()) {
            std::cerr << "UDP组播测试失败" << std::endl;
            return 1;
        }

        std::cout << "所有测试完成" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
}