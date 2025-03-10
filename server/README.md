# Cesium Server

这是一个基于Boost.Beast的高性能Cesium服务器，提供HTTP、WebSocket和UDP组播功能，用于实时地理空间数据传输。

## 功能特点

- **HTTP服务器**：提供RESTful API接口，支持坐标数据的获取和更新
- **WebSocket服务器**：支持实时双向通信，用于坐标数据的实时推送
- **UDP组播服务器**：支持高效的一对多数据分发，适用于广播场景
- **模拟数据生成**：内置模拟数据生成功能，方便测试和演示

## 系统架构

系统由以下主要组件构成：

- `CesiumServerApp`：应用程序主类，协调各服务器组件
- `HttpServer`：基于Boost.Beast的HTTP服务器实现
- `WebSocketServer`：基于Boost.Beast的WebSocket服务器实现
- `UdpMulticastServer`：基于Boost.Asio的UDP组播服务器实现

## 构建要求

- C++17兼容的编译器
- CMake 3.10+
- Boost 1.87.0+
- OpenSSL

## 构建步骤

```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake
cmake ..

# 构建项目
cmake --build .
```

## 使用方法

```bash
# 使用默认配置启动服务器
./cesium_server

# 指定HTTP和WebSocket端口
./cesium_server --http-port 8080 --ws-port 8081
```

## API文档

### HTTP API

- `GET /coordinates` - 获取最新坐标
- `POST /coordinates` - 更新坐标

### WebSocket消息

- `{"type": "ping"}` - 心跳检测
- `{"type": "get_coordinates"}` - 请求当前坐标

### UDP组播

默认组播地址：239.255.0.1:5000

## 性能优化

- 使用异步I/O和事件驱动架构
- 实现消息队列和批处理机制
- 优化内存管理和资源使用
- 增强错误处理和恢复机制

## 功能特点

- HTTP API 用于处理常规请求
- WebSocket 接口用于实时通信
- 支持坐标数据的接收和广播
- 模拟数据生成功能
- 跨域资源共享 (CORS) 支持

## 依赖项

- C++17 兼容的编译器
- CMake 3.10 或更高版本
- Boost 库 (system, thread)
- OpenSSL

## 构建说明

### Windows

1. 安装依赖项：
   - 安装 [Visual Studio](https://visualstudio.microsoft.com/) 或 [MinGW](https://www.mingw-w64.org/)
   - 安装 [CMake](https://cmake.org/download/)
   - 安装 [Boost](https://www.boost.org/users/download/)
   - 安装 [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)

2. 配置环境变量：
   - 设置 `BOOST_ROOT` 指向 Boost 安装目录
   - 设置 `OPENSSL_ROOT_DIR` 指向 OpenSSL 安装目录

3. 构建项目：
   ```
   cd server
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

### Linux

1. 安装依赖项：
   ```
   sudo apt-get update
   sudo apt-get install build-essential cmake libboost-all-dev libssl-dev
   ```

2. 构建项目：
   ```
   cd server
   mkdir build
   cd build
   cmake ..
   make
   ```

## 使用说明

### 启动服务器

```
./cesium_server [options]
```

可用选项：
- `--http-address <address>` - HTTP 服务器地址 (默认: 0.0.0.0)
- `--http-port <port>` - HTTP 服务器端口 (默认: 3000)
- `--ws-address <address>` - WebSocket 服务器地址 (默认: 0.0.0.0)
- `--ws-port <port>` - WebSocket 服务器端口 (默认: 3001)
- `--help` - 显示帮助信息

### HTTP API

#### 获取服务器状态

```
GET /
```

响应示例：
```json
{
  "status": "ok",
  "message": "Cesium Server is running",
  "timestamp": 1646123456789,
  "clients": 2
}
```

#### 发送坐标

```
POST /coordinates
Content-Type: application/json

{
  "longitude": 116.3912,
  "latitude": 39.9073
}
```

响应示例：
```json
{
  "status": "ok",
  "message": "Coordinates received"
}
```

#### 获取最新坐标

```
GET /coordinates
```

响应示例：
```json
{
  "longitude": 116.3912,
  "latitude": 39.9073,
  "timestamp": 1646123456789
}
```

### WebSocket API

连接 URL：`ws://<server-address>:<ws-port>`

#### 客户端消息

##### Ping 消息
```json
{
  "type": "ping"
}
```

##### 获取坐标
```json
{
  "type": "get_coordinates"
}
```

#### 服务器消息

##### 欢迎消息
```json
{
  "type": "welcome",
  "message": "Welcome to Cesium Server",
  "clients": 2,
  "timestamp": 1646123456789
}
```

##### Pong 响应
```json
{
  "type": "pong",
  "timestamp": 1646123456789
}
```

##### 坐标数据
```json
{
  "type": "coordinates",
  "longitude": 116.3912,
  "latitude": 39.9073,
  "timestamp": 1646123456789
}
```

##### 坐标更新广播
```json
{
  "type": "coordinates_update",
  "longitude": 116.3912,
  "latitude": 39.9073,
  "timestamp": 1646123456789
}
```

##### 模拟数据
```json
{
  "type": "simulation_data",
  "longitude": 116.3912,
  "latitude": 39.9073,
  "altitude": 1234.56,
  "timestamp": 1646123456789
}
```

## 与前端集成

在前端项目中，您可以使用以下代码与后端服务器进行交互：

### HTTP 请求示例

```javascript
// 发送坐标
async function sendCoordinates(longitude, latitude) {
  try {
    const response = await fetch('http://localhost:3000/coordinates', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ longitude, latitude })
    });
    
    if (!response.ok) {
      throw new Error('Network response was not ok');
    }
    
    const data = await response.json();
    console.log('Coordinates sent successfully:', data);
  } catch (error) {
    console.error('Error sending coordinates:', error);
  }
}
```

### WebSocket 连接示例

```javascript
// 创建 WebSocket 连接
const ws = new WebSocket('ws://localhost:3001');

// 连接打开时
ws.onopen = () => {
  console.log('WebSocket connection established');
  
  // 发送 ping 消息
  ws.send(JSON.stringify({ type: 'ping' }));
  
  // 请求坐标
  ws.send(JSON.stringify({ type: 'get_coordinates' }));
};

// 接收消息
ws.onmessage = (event) => {
  try {
    const message = JSON.parse(event.data);
    console.log('Received message:', message);
    
    // 处理不同类型的消息
    switch (message.type) {
      case 'welcome':
        console.log(`Connected to server. Total clients: ${message.clients}`);
        break;
        
      case 'pong':
        console.log('Received pong response');
        break;
        
      case 'coordinates':
      case 'coordinates_update':
        console.log(`Received coordinates: ${message.longitude}, ${message.latitude}`);
        // 更新地图上的位置
        break;
        
      case 'simulation_data':
        console.log(`Received simulation data: ${message.longitude}, ${message.latitude}, ${message.altitude}`);
        // 更新模拟数据显示
        break;
    }
  } catch (error) {
    console.error('Error parsing message:', error);
  }
};

// 连接关闭时
ws.onclose = () => {
  console.log('WebSocket connection closed');
};

// 连接错误时
ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

// 创建数据库连接
auto& pool = DatabasePool::getInstance();
pool.init(DatabaseType::MYSQL, "localhost", 3306, "username", "password", "database_name");

// 创建认证服务
auto db = pool.acquire();
AuthService authService(db);

// 用户注册
if (authService.registerUser("test_user", "password123", "user")) {
    std::cout << "用户注册成功" << std::endl;
}

// 用户登录
auto user = authService.login("test_user", "password123");
if (user) {
    std::cout << "登录成功，用户ID: " << user->id << std::endl;
    // 生成token
    std::string token = authService.generateToken(*user);
    // 使用token进行后续操作...
}

// 释放数据库连接
pool.release(db);
```

## 许可证

MIT

### 文件结构

```
server/
├── include/                  # 头文件
│   ├── http_server.h         # HTTP 服务器
│   ├── websocket_server.h    # WebSocket 服务器
│   └── cesium_server_app.h   # 应用程序主类
├── src/                      # 源文件
│   ├── http_server.cpp       # HTTP 服务器实现
│   ├── websocket_server.cpp  # WebSocket 服务器实现
│   ├── cesium_server_app.cpp # 应用程序主类实现
│   └── main.cpp              # 程序入口
├── build/                    # 构建目录
├── CMakeLists.txt            # CMake 构建配置
└── README.md                 # 使用说明
```


- UDP组播服务器优化

- 增加了错误恢复机制，当连接出现问题时能自动重连组播组
- 实现了异步消息队列，提高了消息处理效率
- 添加了缓冲区大小配置，优化了内存使用
- 增强了错误处理能力，提高了服务稳定性
- WebSocket服务器优化

- 改进了消息队列处理机制，确保消息按顺序发送
- 增强了会话管理，提高了连接稳定性
- 优化了广播功能，减少锁竞争
- CesiumServerApp优化

- 完善了配置管理系统，使服务器更易于配置
- 增强了错误处理和恢复机制
- 改进了坐标数据处理和广播功能