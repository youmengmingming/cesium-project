# Cesium 项目后端服务器

这是一个用 C++ 编写的后端服务器，用于与 Cesium 前端项目进行交互。服务器提供 HTTP API 和 WebSocket 接口，支持常规请求和不间断通信。

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