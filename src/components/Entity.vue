<template>
  <div class="entity-container">
    <!-- 实体控制面板，可以根据需要添加UI控件 -->
    <div v-if="showControls" class="entity-controls">
      <div class="connection-status" :class="connectionStatusClass">
        <span>{{ connectionStatusText }}</span>
      </div>
      <div class="entity-count">
        <span>实体数量: {{ entityCount }}</span>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onMounted, onUnmounted, computed, PropType } from 'vue';
import * as Cesium from 'cesium';
import { WebSocketService, ConnectionStatus, EntityData, getWebSocketService, WebSocketEventCallback } from '../utils/websocket';

export default defineComponent({
  name: 'EntityComponent',
  props: {
    websocketUrl: {
      type: String,
      required: true
    },
    websocketInstance: {
      type: Object as PropType<WebSocketService>,
      default: null
    },
    entityTimeout: {
      type: Number,
      default: 10000
    },
    showControls: {
      type: Boolean,
      default: false
    },
    forceNewConnection: {
      type: Boolean,
      default: false
    }
  },
  setup(props, { expose }) {
    // 状态变量
    const connectionStatus = ref<ConnectionStatus>(ConnectionStatus.DISCONNECTED);
    const entityCount = ref(0);
    const entities = ref<Map<string, Cesium.Entity>>(new Map());
    let websocketService: WebSocketService;
    let viewer: Cesium.Viewer;
    const isConnected = ref(false);

    // 计算属性
    const connectionStatusClass = computed(() => {
      switch (connectionStatus.value) {
        case ConnectionStatus.CONNECTED:
          return 'status-connected';
        case ConnectionStatus.CONNECTING:
          return 'status-connecting';
        case ConnectionStatus.ERROR:
          return 'status-error';
        default:
          return 'status-disconnected';
      }
    });

    const connectionStatusText = computed(() => {
      switch (connectionStatus.value) {
        case ConnectionStatus.CONNECTED:
          return '已连接';
        case ConnectionStatus.CONNECTING:
          return '连接中...';
        case ConnectionStatus.ERROR:
          return '连接错误';
        default:
          return '未连接';
      }
    });

    // 初始化WebSocket服务
    const initWebSocket = () => {
      try {
        // 优先使用传入的WebSocket实例
        if (props.websocketInstance) {
          websocketService = props.websocketInstance;
        } else {
          // 否则创建新的WebSocket实例
          websocketService = getWebSocketService(props.websocketUrl, props.entityTimeout, props.forceNewConnection);
        }
        
        // 连接WebSocket
        websocketService.connect();
        
        // 使用新的事件订阅机制
        websocketService.on('statusChange', handleStatusChange);
        websocketService.on('entityUpdate', handleEntityUpdate);
        websocketService.on('entityRemove', handleEntityRemove);
      } catch (error) {
        console.error('初始化WebSocket服务失败:', error);
      }
    };

    // 初始化Cesium Viewer
    const initViewer = () => {
      // 获取全局Cesium Viewer实例
      viewer = (window as any).cesiumViewer;
      if (!viewer) {
        console.error('Cesium Viewer未初始化');
      }
    };

    // 创建或更新实体
    const createOrUpdateEntity = (entityData: EntityData) => {
      const { id, longitude, latitude, height, properties } = entityData;
      const position = Cesium.Cartesian3.fromDegrees(longitude, latitude, height || 0);

      if (entities.value.has(id)) {
        // 更新现有实体
        const entity = entities.value.get(id)!;
        (entity.position as Cesium.PositionProperty).setValue(position);
        
        // 更新实体属性
        if (properties) {
          Object.entries(properties).forEach(([key, value]) => {
            if (entity.properties) {
              entity.properties[key] = new Cesium.ConstantProperty(value);
            }
          });
        }
      } else {
        // 创建新实体
        const entity = viewer.entities.add({
          id,
          position,
          point: {
            pixelSize: 10,
            color: Cesium.Color.YELLOW,
            outlineColor: Cesium.Color.BLACK,
            outlineWidth: 2,
            heightReference: Cesium.HeightReference.CLAMP_TO_GROUND
          },
          label: {
            text: id,
            font: '14px sans-serif',
            style: Cesium.LabelStyle.FILL_AND_OUTLINE,
            outlineWidth: 2,
            verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
            pixelOffset: new Cesium.Cartesian2(0, -10),
            heightReference: Cesium.HeightReference.CLAMP_TO_GROUND
          },
          properties: {}
        });

        // 添加实体属性
        if (properties) {
          Object.entries(properties).forEach(([key, value]) => {
            if (entity.properties) {
              entity.properties[key] = new Cesium.ConstantProperty(value);
            }
          });
        }

        entities.value.set(id, entity);
        entityCount.value = entities.value.size;
      }
    };

    // 移除实体
    const removeEntity = (entityData: EntityData) => {
      const { id } = entityData;
      if (entities.value.has(id)) {
        viewer.entities.removeById(id);
        entities.value.delete(id);
        entityCount.value = entities.value.size;
      }
    };

    // 处理WebSocket状态变更事件
    const handleStatusChange = (event: any) => {
      // 支持新的事件订阅机制和旧的全局事件
      connectionStatus.value = event.status || (event.detail && event.detail.status);
    };

    // 处理实体更新事件
    const handleEntityUpdate = (event: any) => {
      // 支持新的事件订阅机制和旧的全局事件
      const entityData = event.detail ? event.detail as EntityData : event as EntityData;
      createOrUpdateEntity(entityData);
    };

    // 处理实体移除事件
    const handleEntityRemove = (event: any) => {
      // 支持新的事件订阅机制和旧的全局事件
      const entityData = event.detail ? event.detail as EntityData : event as EntityData;
      removeEntity(entityData);
    };

    // 连接WebSocket服务器
    const connect = () => {
      if (isConnected.value) {
        console.log('WebSocket已连接，无需重复连接');
        return;
      }
      
      initViewer();
      initWebSocket();
      
      // 添加全局事件监听
      window.addEventListener('websocketStatusChange', handleStatusChange as EventListener);
      window.addEventListener('entityUpdate', handleEntityUpdate as EventListener);
      window.addEventListener('entityRemove', handleEntityRemove as EventListener);
      
      isConnected.value = true;
    };
    
    // 断开WebSocket连接
    const disconnect = () => {
      if (!isConnected.value) {
        return;
      }
      
      // 移除全局事件监听
      window.removeEventListener('websocketStatusChange', handleStatusChange as EventListener);
      window.removeEventListener('entityUpdate', handleEntityUpdate as EventListener);
      window.removeEventListener('entityRemove', handleEntityRemove as EventListener);
      
      // 移除WebSocket服务事件订阅
      if (websocketService) {
        websocketService.off('statusChange', handleStatusChange);
        websocketService.off('entityUpdate', handleEntityUpdate);
        websocketService.off('entityRemove', handleEntityRemove);
        websocketService.disconnect();
      }

      // 清理实体
      entities.value.forEach((entity) => {
        viewer.entities.remove(entity);
      });
      entities.value.clear();
      entityCount.value = 0;
      isConnected.value = false;
    };
    
    // 组件挂载时只初始化Viewer，不自动连接WebSocket
    onMounted(() => {
      initViewer();
    });

    // 组件卸载时
    onUnmounted(() => {
      // 移除全局事件监听
      window.removeEventListener('websocketStatusChange', handleStatusChange as EventListener);
      window.removeEventListener('entityUpdate', handleEntityUpdate as EventListener);
      window.removeEventListener('entityRemove', handleEntityRemove as EventListener);
      
      // 移除WebSocket服务事件订阅
      if (websocketService) {
        websocketService.off('statusChange', handleStatusChange);
        websocketService.off('entityUpdate', handleEntityUpdate);
        websocketService.off('entityRemove', handleEntityRemove);
      }

      // 清理实体
      entities.value.forEach((entity) => {
        viewer.entities.remove(entity);
      });
      entities.value.clear();
    });

    // 向外部暴露方法和状态
    expose({
      connect,
      disconnect
    });
    
    return {
      connectionStatus,
      connectionStatusClass,
      connectionStatusText,
      entityCount
    };
  }
});
</script>

<style scoped>
.entity-container {
  position: absolute;
  z-index: 100;
}

.entity-controls {
  position: absolute;
  top: 70px;
  right: 10px;
  background: var(--bg-dark);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-sm);
  padding: 10px;
  color: var(--text-light);
  font-size: 14px;
  backdrop-filter: blur(8px);
  box-shadow: var(--shadow-sm);
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.connection-status {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 8px;
  border-radius: 4px;
}

.connection-status::before {
  content: '';
  display: inline-block;
  width: 10px;
  height: 10px;
  border-radius: 50%;
}

.status-connected {
  background: rgba(46, 204, 113, 0.2);
}

.status-connected::before {
  background: var(--accent-color);
}

.status-connecting {
  background: rgba(243, 156, 18, 0.2);
}

.status-connecting::before {
  background: var(--warning-color);
}

.status-disconnected, .status-error {
  background: rgba(231, 76, 60, 0.2);
}

.status-disconnected::before, .status-error::before {
  background: var(--danger-color);
}

.entity-count {
  padding: 4px 8px;
  background: rgba(52, 152, 219, 0.2);
  border-radius: 4px;
}

@media (max-width: 768px) {
  .entity-controls {
    top: 80px;
    right: 10px;
    font-size: 12px;
    padding: 8px;
  }
}
</style>