# ZeroMQ 服务器使用指南

本文档介绍了 Cesium 服务器中 ZeroMQ 组件的功能和使用方法。ZeroMQ 是一个高性能的异步消息传递库，支持多种通信模式，适用于分布式系统和微服务架构。

## 功能特点

- 支持多种通信模式：请求-响应(REQ-REP)、发布-订阅(PUB-SUB)、推送-拉取(PUSH-PULL)
- 高性能异步消息处理
- 与 Cesium 服务器无缝集成
- 支持坐标数据的实时传输和更新

## 配置选项

ZeroMQ 服务器可以通过 `ServerConfig` 结构进行配置：

```cpp
// ZeroMQ服务器配置
std::string zmq_address;         // 服务器地址，默认为 "0.0.0.0"
unsigned short zmq_port;         // 服务器端口，默认为 5555
ZeroMQServer::Mode zmq_mode;     // 通信模式，默认为 REQ_REP
int zmq_io_threads;              // IO线程数，默认为 1
bool enable_zmq;                 // 是否启用 ZeroMQ 服务器，默认为 true
```

## 通信模式

### 请求-响应模式 (REQ-REP)

请求-响应模式是一种同步通信模式，客户端发送请求，服务器处理请求并返回响应。这种模式适用于需要确认的操作，如获取坐标数据或更新坐标。

**服务器端示例：**

```cpp
// 创建 ZeroMQ 服务器（REQ-REP 模式）
ZeroMQServer server("0.0.0.0", 5555, ZeroMQServer::Mode::REQ_REP);

// 设置消息处理器
server.setMessageHandler([](const std::string& message, const std::string& topic) {
    // 解析消息
    auto msg = json::parse(message);
    
    // 处理请求
    if (msg.is_object() && msg.as_object().contains("type")) {
        std::string type = msg.as_object()["type"].as_string().c_str();
        
        if (type == "get_coordinates") {
            // 创建响应
            json::object response;
            response["type"] = "coordinates";
            response["longitude"] = 116.3912;
            response["latitude"] = 39.9073;
            
            // 发送响应
            server.sendMessage(json::serialize(response));
        }
    }
});

// 启动服务器
server.run();
```

**客户端示例：**

```cpp
// 创建 ZeroMQ 上下文
zmq::context_t context(1);

// 创建请求套接字
zmq::socket_t socket(context, ZMQ_REQ);

// 连接到服务器
socket.connect("tcp://localhost:5555");

// 创建请求消息
std::string request = "{\"type\":\"get_coordinates\"}";

// 发送请求
socket.send(zmq::buffer(request), zmq::send_flags::none);

// 接收响应
zmq::message_t reply;
socket.recv(reply, zmq::recv_flags::none);
std::string response(static_cast<char*>(reply.data()), reply.size());
```

### 发布-订阅模式 (PUB-SUB)

发布-订阅模式是一种异步通信模式，服务器发布消息，客户端订阅感兴趣的主题并接收相关消息。这种模式适用于广播场景，如坐标数据的实时更新。

**服务器端示例：**

```cpp
// 创建 ZeroMQ 服务器（PUB-SUB 模式）
ZeroMQServer server("0.0.0.0", 5556, ZeroMQServer::Mode::PUB_SUB);

// 启动服务器
server.run();

// 定期发布坐标更新
while (true) {
    // 创建消息
    json::object message;
    message["type"] = "coordinates_update";
    message["longitude"] = 116.3912;
    message["latitude"] = 39.9073;
    
    // 发送消息（指定主题为 "coordinates"）
    server.sendMessage(json::serialize(message), "coordinates");
    
    // 等待一秒
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

**客户端示例：**

```cpp
// 创建 ZeroMQ 上下文
zmq::context_t context(1);

// 创建订阅套接字
zmq::socket_t socket(context, ZMQ_SUB);

// 连接到服务器
socket.connect("tcp://localhost:5556");

// 设置订阅过滤器（订阅 "coordinates" 主题）
socket.set(zmq::sockopt::subscribe, "coordinates");

// 接收消息
while (true) {
    zmq::message_t message;
    socket.recv(message, zmq::recv_flags::none);
    std::string data(static_cast<char*>(message.data()), message.size());
    
    // 处理消息
    // ...
}
```

### 推送-拉取模式 (PUSH-PULL)

推送-拉取模式是一种异步通信模式，服务器推送消息到工作队列，客户端从队列中拉取消息进行处理。这种模式适用于任务分发场景，如模拟数据的生成和处理。

**服务器端示例：**

```cpp
// 创建 ZeroMQ 服务器（PUSH-PULL 模式）
ZeroMQServer server("0.0.0.0", 5557, ZeroMQServer::Mode::PUSH_PULL);

// 启动服务器
server.run();

// 定期推送模拟数据
while (true) {
    // 创建消息
    json::object message;
    message["type"] = "simulation_data";
    message["longitude"] = 116.3912;
    message["latitude"] = 39.9073;
    message["altitude"] = 1000.0;
    
    // 发送消息
    server.sendMessage(json::serialize(message));
    
    // 等待一秒
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

**客户端示例：**

```cpp
// 创建 ZeroMQ 上下文
zmq::context_t context(1);

// 创建拉取套接字
zmq::socket_t socket(context, ZMQ_PULL);

// 连接到服务器
socket.connect("tcp://localhost:5557");

// 接收消息
while (true) {
    zmq::message_t message;
    socket.recv(message, zmq::recv_flags::none);
    std::string data(static_cast<char*>(message.data()), message.size());
    
    // 处理消息
    // ...
}
```

## 与 Cesium 服务器集成

ZeroMQ 服务器已经与 Cesium 服务器无缝集成，可以通过以下方式使用：

1. 在 `ServerConfig` 中配置 ZeroMQ 服务器参数
2. 创建 `CesiumServerApp` 实例时传入配置
3. 调用 `run()` 方法启动服务器

```cpp
// 创建服务器配置
cesium_server::ServerConfig config;
config.zmq_address = "0.0.0.0";
config.zmq_port = 5555;
config.zmq_mode = cesium_server::ZeroMQServer::Mode::REQ_REP;
config.zmq_io_threads = 1;
config.enable_zmq = true;

// 创建服务器应用程序
cesium_server::CesiumServerApp app(config);

// 启动服务器
app.run();
```

## 示例代码

完整的示例代码可以在 `examples/zeromq_examples.cpp` 文件中找到，包括三种通信模式的服务器和客户端实现。

### 编译和运行示例

```bash
# 编译示例
g++ -o zeromq_examples examples/zeromq_examples.cpp -lzmq -std=c++17

# 运行服务器（请求-响应模式）
./zeromq_examples server req-rep

# 运行客户端（请求-响应模式）
./zeromq_examples client req-rep

# 运行服务器（发布-订阅模式）
./zeromq_examples server pub-sub

# 运行客户端（发布-订阅模式）
./zeromq_examples client pub-sub

# 运行服务器（推送-拉取模式）
./zeromq_examples server push-pull

# 运行客户端（推送-拉取模式）
./zeromq_examples client push-pull
```

## 性能优化

- 使用异步消息队列，提高消息处理效率
- 优化套接字选项，减少资源占用
- 使用轮询方式接收消息，避免阻塞
- 多线程处理，提高并发性能

## 注意事项

- ZeroMQ 服务器需要安装 libzmq 和 cppzmq 库
- 不同通信模式适用于不同场景，请根据需求选择合适的模式
- 在高并发场景下，建议增加 IO 线程数
- 消息处理器中的异常应该被捕获并处理，避免影响服务器运行