const WebSocket = require('ws');
const _ = require('lodash');

// 生成10个测试实体
const generateEntities = () => {
  return Array.from({length: 10}, (_, i) => ({
    id: `entity-${i+1}`,
    shipName: `测试船只${i+1}`, 
    shipNumber: `TSH00${i+1}`,
    longitude: 116.397 + (Math.random() - 0.5) * 0.1,
    latitude: 39.909 + (Math.random() - 0.5) * 0.1,
    height: 50 + Math.random() * 50,
    heading: Math.random() * 360,
    country: ['中国', '美国', '日本'][i%3],
    type: ['货轮', '油轮', '军舰'][i%3],
    attr: i%2,
    time: Date.now()
  }));
};

const wss = new WebSocket.Server({ port: 3001 });

wss.on('connection', (ws) => {
  console.log('客户端连接成功');
  
  // 发送初始实体数据
  const entities = generateEntities();
  ws.send(JSON.stringify(entities));

  // 定时更新位置数据（每3秒）
  const updateInterval = setInterval(() => {
    entities.forEach(entity => {
      entity.longitude += (Math.random() - 0.5) * 0.001;
      entity.latitude += (Math.random() - 0.5) * 0.001;
      entity.heading = Math.random() * 360;
      entity.time = Date.now();
    });
    ws.send(JSON.stringify(entities));
  }, 500);

  // 心跳检测（每10秒）
  const heartbeatInterval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.ping();
    }
  }, 10000);

  ws.on('close', () => {
    clearInterval(updateInterval);
    clearInterval(heartbeatInterval);
    console.log('客户端断开连接');
  });
});

console.log('WebSocket服务端运行在 ws://localhost:3001');