<template>
  <div class="menu-bar">
    <div class="left-section">
      <div class="logo">
        <img src="../assets/logo.png" alt="Logo">
      </div>
      <div class="buttons">
        <button @click="handleTest1Click" class="menu-button">
          <i class="fas fa-chart-line"></i>
          测试1
        </button>
        <button @click="handleTest2Click" class="menu-button">
          <i class="fas fa-satellite"></i>
          测试2
        </button>
        <button v-for="i in 2" :key="i+2" class="menu-button">
          <i class="fas fa-layer-group"></i>
          测试{{ i+2 }}
        </button>
      </div>
    </div>
    <div class="right-section">
      <Entity
        ref="entityComponent"
        :websocket-url="'ws://localhost:3001'"
        :show-controls="true"
        class="entity-panel"
      />
      <div class="date-time">
        <div class="date">{{ currentDate }}</div>
        <div class="time">{{ currentTime }}</div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onMounted, onUnmounted } from 'vue'
import Entity from './Entity.vue'
import { WebSocketService } from '../utils/websocket'

interface Coordinates {
  longitude: number;
  latitude: number;
}

export default defineComponent({
  name: 'MenuBar',
  components: {
    Entity
  },
  setup() {
    const currentTime = ref('')
    const currentDate = ref('')
    let timer: NodeJS.Timeout
    const currentCoords = ref<Coordinates | null>(null)

    const updateTime = () => {
      const now = new Date()
      // 设置时间
      currentTime.value = now.toLocaleTimeString('zh-CN', {
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
      })
      // 设置日期
      currentDate.value = now.toLocaleDateString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit'
      })
    }

    onMounted(() => {
      updateTime()
      timer = setInterval(updateTime, 1000)

      // 添加坐标更新事件监听
      window.addEventListener('updateCoordinates', ((event: CustomEvent) => {
        const { longitude, latitude } = event.detail;
        updateCoordinates(longitude, latitude);
      }) as EventListener);
    })

    onUnmounted(() => {
      clearInterval(timer)
      // 移除事件监听
      window.removeEventListener('updateCoordinates', ((event: CustomEvent) => {
        const { longitude, latitude } = event.detail;
        updateCoordinates(longitude, latitude);
      }) as EventListener);
    })

    const handleTest1Click = async () => {
      if (!currentCoords.value) {
        console.error('No coordinates available');
        return;
      }

      try {
        const response = await fetch('http://127.0.0.1:3000/coordinates', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(currentCoords.value)
        });

        if (!response.ok) {
          throw new Error('Network response was not ok');
        }

        console.log('Coordinates sent successfully');
      } catch (error) {
        console.error('Error:', error);
      }
    };

    // 添加更新坐标的方法
    const updateCoordinates = (longitude: number, latitude: number) => {
      currentCoords.value = { longitude, latitude };
    };

    // 添加对Entity组件的引用
    const entityComponent = ref(null);
    
    // 测试2按钮点击处理函数
    const handleTest2Click = () => {
      if (entityComponent.value) {
        // 调用Entity组件的connect方法连接WebSocket服务器
        entityComponent.value.connect();
        console.log('已触发WebSocket连接');
      } else {
        console.error('Entity组件未找到');
      }
    };
    
    return {
      currentTime,
      currentDate,
      handleTest1Click,
      handleTest2Click,
      currentCoords,
      updateCoordinates,
      entityComponent
    }
  }
})
</script>

<style scoped>
.menu-bar {
  position: fixed;
  top: 0px;
  left: 0px;
  right: 0px;
  height: 54px;
  background: var(--bg-dark);
  backdrop-filter: blur(12px);
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 20px;
  z-index: 1000;
}

.left-section {
  display: flex;
  align-items: center;
  gap: 32px;
}

.logo {
  height: 40px;
  width: 40px;
  display: flex;
  align-items: center;
  transition: transform var(--transition-normal);
}

.logo:hover {
  transform: scale(1.05);
}

.logo img {
  height: 100%;
  width: 100%;
  object-fit: contain;
}

.buttons {
  display: flex;
  gap: 12px;
}

.menu-button {
  background: transparent;
  outline: none; 
  border: none;
  color: var(--text-light);
  padding: 8px 16px;
  font-size: 14px;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 8px;
}

.menu-button i {
  font-size: 14px;
  color: var(--primary-color);
}

.menu-button:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: var(--border-hover);
}

.menu-button:hover i {
  color: var(--accent-color);

}

.menu-button:active {

}

.right-section {
  display: flex;
  align-items: center;
  gap: 16px;
}

.date-time {
  color: var(--text-light);
  font-size: 14px;
  padding: 8px 16px;
  background: transparent;
  transition: all var(--transition-normal);
  display: flex;
  align-items: center;
  gap: 10px;
}

.date-time i {
  font-size: 15px;
  font-weight: 600;
  color: var(--primary-color);
}

.date {

}

.time {

}

.coord-label {
  font-size: 12px;
  color: var(--text-muted);
}

.coord-value {
  font-size: 14px;
  font-weight: 600;
}

.time:hover, .date:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: var(--border-hover);
  transform: translateY(-2px);
  box-shadow: var(--shadow-sm);
}

.time:hover i, .date:hover i {
  color: var(--accent-color);
  transform: scale(1.1);
}

@media (max-width: 768px) {
  .menu-bar {
    top: 12px;
    left: 12px;
    right: 12px;
    padding: 0 12px;
    height: 56px;
  }

  .menu-button {
    padding: 6px 12px;
    font-size: 13px;
  }

  .time, .coordinates {
    font-size: 13px;
    padding: 6px 12px;
  }

  .logo {
    height: 32px;
    width: 32px;
  }
}
</style>
