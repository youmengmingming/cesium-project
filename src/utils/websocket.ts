import * as Cesium from 'cesium';

// 实体数据接口
export interface EntityData {
  id: string;           // 实体唯一标识
  shipName: string;     // 船舶名称
  shipNumber: string;   // 船舶编号
  longitude: number;    // 经度
  latitude: number;     // 纬度
  height: number;       // 高度
  heading?: number;     // 航向（可选）
  country?: string;     // 国家（可选）
  type?: string;        // 类型（可选）
  attr?: number;        // 敌我属性（可选）
  time: number;         // 最后更新时间戳
}

// WebSocket连接状态
export enum ConnectionStatus {
  CONNECTING = 'connecting',
  CONNECTED = 'connected',
  DISCONNECTED = 'disconnected',
  ERROR = 'error'
}

// 事件类型定义
export type WebSocketEventType = 'statusChange' | 'entityUpdate' | 'entityRemove';
export type WebSocketEventCallback = (data: any) => void;

/**
 * WebSocket服务类 - 用于连接WebSocket服务器并处理实体数据
 */
export class WebSocketService {
  private socket: WebSocket | null = null;
  private url: string;
  private reconnectAttempts: number = 0;
  private maxReconnectAttempts: number = 5;
  private reconnectInterval: number = 3000; // 重连间隔，单位毫秒
  private reconnectTimeoutId: number | null = null;
  private entityDataMap: Map<string, EntityData> = new Map();
  private entityTimeoutMap: Map<string, number> = new Map(); // 存储实体超时计时器ID
  private entityTimeout: number = 10000; // 实体超时时间，单位毫秒
  private status: ConnectionStatus = ConnectionStatus.DISCONNECTED;
  private eventListeners: Map<WebSocketEventType, Set<WebSocketEventCallback>> = new Map();
  private instanceId: string = Math.random().toString(36).substring(2, 9); // 实例唯一标识

  /**
   * 构造函数
   * @param url WebSocket服务器URL
   * @param entityTimeout 实体超时时间（毫秒），默认10秒
   */
  constructor(url: string, entityTimeout: number = 10000) {
    this.url = url;
    this.entityTimeout = entityTimeout;
  }

  /**
   * 连接WebSocket服务器
   */
  public connect(): void {
    if (this.socket && (this.socket.readyState === WebSocket.OPEN || this.socket.readyState === WebSocket.CONNECTING)) {
      console.log('WebSocket已连接或正在连接中');
      return;
    }

    this.status = ConnectionStatus.CONNECTING;
    this.dispatchStatusEvent();

    try {
      this.socket = new WebSocket(this.url);

      this.socket.onopen = this.handleOpen.bind(this);
      this.socket.onmessage = this.handleMessage.bind(this);
      this.socket.onclose = this.handleClose.bind(this);
      this.socket.onerror = this.handleError.bind(this);
    } catch (error) {
      console.error('WebSocket连接错误:', error);
      this.status = ConnectionStatus.ERROR;
      this.dispatchStatusEvent();
      this.attemptReconnect();
    }
  }

  /**
   * 断开WebSocket连接
   */
  public disconnect(): void {
    if (this.socket) {
      this.socket.close();
      this.socket = null;
    }

    // 清除所有实体超时计时器
    this.entityTimeoutMap.forEach((timeoutId) => {
      window.clearTimeout(timeoutId);
    });
    this.entityTimeoutMap.clear();
    
    // 清除重连计时器
    if (this.reconnectTimeoutId !== null) {
      window.clearTimeout(this.reconnectTimeoutId);
      this.reconnectTimeoutId = null;
    }

    this.status = ConnectionStatus.DISCONNECTED;
    this.dispatchStatusEvent();
  }

  /**
   * 获取当前连接状态
   */
  public getStatus(): ConnectionStatus {
    return this.status;
  }

  /**
   * 获取所有实体数据
   */
  public getAllEntityData(): EntityData[] {
    return Array.from(this.entityDataMap.values());
  }

  /**
   * 获取指定ID的实体数据
   * @param id 实体ID
   */
  public getEntityData(id: string): EntityData | undefined {
    return this.entityDataMap.get(id);
  }

  /**
   * 处理WebSocket连接打开事件
   */
  private handleOpen(): void {
    console.log('WebSocket连接成功');
    this.reconnectAttempts = 0;
    this.status = ConnectionStatus.CONNECTED;
    this.dispatchStatusEvent();
  }

  /**
   * 处理WebSocket消息事件
   */
  private handleMessage(event: MessageEvent): void {
    try {
      const data = JSON.parse(event.data);
      
      // 检查数据是否为数组（批量更新）或单个对象
      if (Array.isArray(data)) {
        data.forEach(item => this.processEntityData(item));
      } else {
        this.processEntityData(data);
      }
    } catch (error) {
      console.error('解析WebSocket消息失败:', error);
    }
  }

  /**
   * 处理WebSocket关闭事件
   */
  private handleClose(event: CloseEvent): void {
    console.log(`WebSocket连接关闭: ${event.code} ${event.reason}`);
    this.status = ConnectionStatus.DISCONNECTED;
    this.dispatchStatusEvent();
    this.attemptReconnect();
  }

  /**
   * 处理WebSocket错误事件
   */
  private handleError(event: Event): void {
    console.error('WebSocket错误:', event);
    this.status = ConnectionStatus.ERROR;
    this.dispatchStatusEvent();
  }

  /**
   * 尝试重新连接
   */
  private attemptReconnect(): void {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.log('达到最大重连次数，停止重连');
      return;
    }

    this.reconnectAttempts++;
    console.log(`尝试重连 (${this.reconnectAttempts}/${this.maxReconnectAttempts})...`);

    // 清除之前的重连计时器
    if (this.reconnectTimeoutId !== null) {
      window.clearTimeout(this.reconnectTimeoutId);
    }

    // 设置新的重连计时器
    this.reconnectTimeoutId = window.setTimeout(() => {
      this.connect();
    }, this.reconnectInterval);
  }

  /**
   * 处理实体数据
   * @param data 实体数据
   */
  private processEntityData(data: any): void {
    // 验证必要字段和数据类型
    if (!data.id || 
        typeof data.id !== 'string' || 
        typeof data.longitude !== 'number' || 
        typeof data.latitude !== 'number' || 
        isNaN(data.longitude) || 
        isNaN(data.latitude)) {
      console.error('实体数据格式无效:', data);
      return;
    }

    const now = Date.now();
    const entityData: EntityData = {
      id: data.id,
      shipName: data.shipName || '未知船只',
      shipNumber: data.shipNumber || 'N/A',
      longitude: data.longitude,
      latitude: data.latitude,
      height: typeof data.height === 'number' ? data.height : 0,
      heading: data.heading,
      country: data.country,
      type: data.type,
      attr: data.attr,
      time: Date.now()
    };

    // 更新实体数据
    this.entityDataMap.set(entityData.id, entityData);

    // 清除之前的超时计时器（如果存在）
    if (this.entityTimeoutMap.has(entityData.id)) {
      window.clearTimeout(this.entityTimeoutMap.get(entityData.id));
    }

    // 设置新的超时计时器
    const timeoutId = window.setTimeout(() => {
      this.removeEntity(entityData.id);
    }, this.entityTimeout);

    this.entityTimeoutMap.set(entityData.id, timeoutId);

    // 触发实体更新事件
    this.dispatchEntityEvent('entityUpdate', entityData);
  }

  /**
   * 移除实体
   * @param id 实体ID
   */
  private removeEntity(id: string): void {
    if (this.entityDataMap.has(id)) {
      const entityData = this.entityDataMap.get(id)!;
      this.entityDataMap.delete(id);
      this.entityTimeoutMap.delete(id);
      
      // 触发实体移除事件
      this.dispatchEntityEvent('entityRemove', entityData);
    }
  }

  /**
   * 订阅事件
   * @param eventType 事件类型
   * @param callback 回调函数
   */
  public on(eventType: WebSocketEventType, callback: WebSocketEventCallback): void {
    if (!this.eventListeners.has(eventType)) {
      this.eventListeners.set(eventType, new Set());
    }
    this.eventListeners.get(eventType)?.add(callback);
  }

  /**
   * 取消订阅事件
   * @param eventType 事件类型
   * @param callback 回调函数
   */
  public off(eventType: WebSocketEventType, callback: WebSocketEventCallback): void {
    const listeners = this.eventListeners.get(eventType);
    if (listeners) {
      listeners.delete(callback);
    }
  }

  /**
   * 触发实体事件
   * @param eventName 事件名称
   * @param data 事件数据
   */
  private dispatchEntityEvent(eventName: string, data: EntityData): void {
    // 同时支持新的事件订阅机制和旧的全局事件
    const eventType = eventName === 'entityUpdate' ? 'entityUpdate' : 'entityRemove';
    const listeners = this.eventListeners.get(eventType);
    if (listeners) {
      listeners.forEach(callback => callback(data));
    }
    
    // 为了向后兼容，仍然触发全局事件
    const event = new CustomEvent(eventName, { detail: data });
    window.dispatchEvent(event);
  }

  /**
   * 触发状态变更事件
   */
  private dispatchStatusEvent(): void {
    // 使用新的事件订阅机制
    const listeners = this.eventListeners.get('statusChange');
    if (listeners) {
      listeners.forEach(callback => callback({ status: this.status }));
    }
    
    // 为了向后兼容，仍然触发全局事件
    const event = new CustomEvent('websocketStatusChange', { 
      detail: { status: this.status } 
    });
    window.dispatchEvent(event);
  }
}

// 管理多个WebSocket实例
const websocketInstances: Map<string, WebSocketService> = new Map();

/**
 * 获取WebSocket服务实例
 * @param url WebSocket服务器URL
 * @param entityTimeout 实体超时时间（毫秒）
 * @param forceNew 是否强制创建新实例
 */
export function getWebSocketService(url?: string, entityTimeout?: number, forceNew: boolean = false): WebSocketService {
  // 如果没有提供URL，尝试返回第一个可用的实例
  if (!url) {
    const firstInstance = websocketInstances.values().next().value;
    if (!firstInstance) {
      throw new Error('WebSocket服务未初始化，请提供URL');
    }
    return firstInstance;
  }
  
  // 检查是否已存在相同URL的实例
  const existingInstance = Array.from(websocketInstances.values())
    .find(instance => (instance as any).url === url);
  
  // 如果存在实例且不强制创建新实例，则返回现有实例
  if (existingInstance && !forceNew) {
    return existingInstance;
  }
  
  // 创建新实例
  const newInstance = new WebSocketService(url, entityTimeout);
  websocketInstances.set((newInstance as any).instanceId, newInstance);
  return newInstance;
}

/**
 * 重置指定的WebSocket服务实例
 * @param instance WebSocket服务实例
 */
export function resetWebSocketService(instance?: WebSocketService): void {
  if (instance) {
    // 重置特定实例
    instance.disconnect();
    websocketInstances.delete((instance as any).instanceId);
  } else {
    // 重置所有实例
    websocketInstances.forEach(instance => {
      instance.disconnect();
    });
    websocketInstances.clear();
  }
}