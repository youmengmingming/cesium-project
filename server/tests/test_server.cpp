#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/multicast.hpp>
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
		ioc_(), resolver_(net::make_strand(ioc_)), zmq_context_(1) {
	}

	// Test HTTP server
	bool testHttpServer() {
		try {
			std::cout << "Testing HTTP server..." << std::endl;

			// Test GET /coordinates
			auto getResult = testHttpGet("/coordinates");
			std::cout << "GET /coordinates response: " << getResult << std::endl;

			// Test POST /coordinates
			std::string postData = "{\"longitude\": 116.3912, \"latitude\": 39.9073}";
			auto postResult = testHttpPost("/coordinates", postData);
			std::cout << "POST /coordinates response: " << postResult << std::endl;

			return true;
		}
		catch (const std::exception& e) {
			std::cout << "HTTP test failed: " << e.what() << std::endl;
			return false;
		}
	}

	// Test WebSocket server
	bool testWebSocketServer() {
		try {
			std::cout << "Testing WebSocket server..." << std::endl;

			// Create WebSocket connection
			tcp::socket socket{ ioc_ };
			auto const results = resolver_.resolve(host_, std::to_string(ws_port_));
			net::connect(socket, results.begin(), results.end());

			websocket::stream<tcp::socket> ws{ std::move(socket) };
			ws.handshake(host_, "/");

			// Send ping message
			std::string ping_msg = "{\"type\": \"ping\"}";
			ws.write(net::buffer(ping_msg));

			// Receive pong response
			beast::flat_buffer buffer;
			ws.read(buffer);
			std::cout << "WebSocket response: " << beast::make_printable(buffer.data()) << std::endl;

			// Request coordinates data
			std::string coords_msg = "{\"type\": \"get_coordinates\"}";
			ws.write(net::buffer(coords_msg));

			// Receive coordinates response
			buffer.consume(buffer.size());
			ws.read(buffer);
			std::cout << "Coordinates data response: " << beast::make_printable(buffer.data()) << std::endl;

			// Close connection normally
			ws.close(websocket::close_code::normal);

			return true;
		}
		catch (const std::exception& e) {
			std::cout << "WebSocket test failed: " << e.what() << std::endl;
			return false;
		}
	}

	// Test ZMQ request-response mode
	bool testZmqReqRep() {
		try {
			std::cout << "Testing ZMQ request-response mode..." << std::endl;

			// Create request socket
			zmq::socket_t socket(zmq_context_, ZMQ_REQ);
			socket.connect("tcp://localhost:5555");

			// Send request
			std::string request = "{\"type\":\"get_coordinates\"}";
			std::cout << "Sending request: " << request << std::endl;
			socket.send(zmq::buffer(request), zmq::send_flags::none);

			// Receive response
			zmq::message_t reply;
			auto result = socket.recv(reply, zmq::recv_flags::none);

			if (result) {
				std::string response(static_cast<char*>(reply.data()), reply.size());
				std::cout << "Received response: " << response << std::endl;
				return true;
			}

			return false;
		}
		catch (const std::exception& e) {
			std::cout << "ZMQ request-response test failed: " << e.what() << std::endl;
			return false;
		}
	}

	// Test ZMQ publish-subscribe mode
	bool testZmqPubSub() {
		try {
			std::cout << "Testing ZMQ publish-subscribe mode..." << std::endl;

			// Create subscriber socket
			zmq::socket_t socket(zmq_context_, ZMQ_SUB);
			socket.connect("tcp://localhost:5555");
			socket.set(zmq::sockopt::subscribe, "simulation");

			// Receive message
			std::cout << "Waiting for messages..." << std::endl;
			zmq::message_t message;
			auto result = socket.recv(message, zmq::recv_flags::none);

			if (result) {
				std::string data(static_cast<char*>(message.data()), message.size());
				std::cout << "Received message: " << data << std::endl;
				return true;
			}

			return false;
		}
		catch (const std::exception& e) {
			std::cout << "ZMQ publish-subscribe test failed: " << e.what() << std::endl;
			return false;
		}
	}

	// Test UDP multicast
	bool testUdpMulticast() {
		try {
			std::cout << "Testing UDP multicast..." << std::endl;

			// Create UDP socket
			udp::socket socket(ioc_);
			socket.open(udp::v4());

			// Join multicast group
			socket.set_option(udp::socket::reuse_address(true));
			socket.bind(udp::endpoint(udp::v4(), 5000));

			net::ip::address multicast_address = net::ip::make_address("239.255.0.1");
			socket.set_option(net::ip::multicast::join_group(multicast_address));

			// Receive data
			std::array<char, 1024> recv_buf;
			udp::endpoint sender_endpoint;

			// Set receive timeout
			socket.async_receive_from(
				net::buffer(recv_buf), sender_endpoint,
				[&](boost::system::error_code ec, std::size_t length) {
					if (!ec) {
						std::cout << "Received UDP multicast data: "
							<< std::string(recv_buf.data(), length) << std::endl;
					}
				});

			// Run IO service
			ioc_.run_for(std::chrono::seconds(5));

			return true;
		}
		catch (const std::exception& e) {
			std::cout << "UDP multicast test failed: " << e.what() << std::endl;
			return false;
		}
	}

private:
	// HTTP GET request
	std::string testHttpGet(const std::string& target) {
		tcp::socket socket{ ioc_ };
		auto const results = resolver_.resolve(host_, std::to_string(http_port_));
		net::connect(socket, results.begin(), results.end());

		http::request<http::string_body> req{ http::verb::get, target, 11 };
		req.set(http::field::host, host_);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		http::write(socket, req);

		beast::flat_buffer buffer;
		http::response<http::string_body> res;
		http::read(socket, buffer, res);

		return res.body();
	}

	// HTTP POST request
	std::string testHttpPost(const std::string& target, const std::string& data) {
		tcp::socket socket{ ioc_ };
		auto const results = resolver_.resolve(host_, std::to_string(http_port_));
		net::connect(socket, results.begin(), results.end());

		http::request<http::string_body> req{ http::verb::post, target, 11 };
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

		std::cout << "Starting server tests..." << std::endl;

		// Test UDP multicast
		if (!test.testUdpMulticast())
		{
			std::cout << "UDP multicast test failed" << std::endl;
			return 1;
		}

		// Test HTTP server
		if (!test.testHttpServer())
		{
			std::cout << "HTTP server test failed" << std::endl;
			return 1;
		}

		if (!test.testWebSocketServer())
		{
			std::cout << "WebSocket server test failed" << std::endl;
			return 1;
		}

		// Test HTTP server
		if (!test.testHttpServer())
		{
			std::cout << "HTTP server test failed" << std::endl;
			return 1;
		}

		// Test ZMQ publish-subscribe mode
		if (!test.testZmqPubSub()) {
			std::cout << "ZMQ publish-subscribe test failed" << std::endl;
			return 1;
		}

		// Test ZMQ request-response mode
		if (!test.testZmqReqRep()) {
			std::cout << "ZMQ request-response test failed" << std::endl;
			return 1;
		}









		// Test WebSocket server

		//std::cout << "All tests completed" << std::endl;
		//std::cout << "All tests completed" << std::endl;
		return 0;
	}
	catch (const std::exception& e) {
		std::cout << "Error occurred during testing: " << e.what() << std::endl;
		return 1;
	}
}