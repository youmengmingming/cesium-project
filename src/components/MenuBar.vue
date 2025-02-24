<template>
  <div class="menu-bar">
    <div class="left-section">
      <div class="logo">
        <img src="../assets/logo.png" alt="Logo">
      </div>
      <div class="buttons">
        <button @click="handleTest1Click" class="menu-button">测试1</button>
        <button v-for="i in 3" :key="i+1" class="menu-button">
          测试{{ i+1 }}
        </button>
      </div>
    </div>
    <div class="right-section">
      <div class="coordinates" v-if="currentCoords">
        经度: {{ currentCoords.longitude.toFixed(6) }}
        纬度: {{ currentCoords.latitude.toFixed(6) }}
      </div>
      <div class="time">
        <div class="date">{{ currentDate }}</div>
        <div class="time-text">{{ currentTime }}</div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onMounted, onUnmounted } from 'vue'

interface Coordinates {
  longitude: number;
  latitude: number;
}

export default defineComponent({
  name: 'MenuBar',
  setup() {
    const currentTime = ref('')
    const currentDate = ref('')
    let timer: number
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

    return {
      currentTime,
      currentDate,
      handleTest1Click,
      currentCoords,
      updateCoordinates
    }
  }
})
</script>

<style scoped>
.menu-bar {
  position: fixed;
  top: 10px;
  left: 15px;
  right: 15px;
  height: 60px;
  background: linear-gradient(to right, rgba(28, 32, 38, 0.95), rgba(28, 32, 38, 0.85));
  backdrop-filter: blur(10px);
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 16px;
  z-index: 1000;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.2);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 6px;
}

.left-section {
  display: flex;
  align-items: center;
  gap: 30px;
}

.logo {
  height: 36px;
  width: 36px;
  display: flex;
  align-items: center;
  transition: transform 0.3s ease;
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
  border: 1px solid rgba(255, 255, 255, 0.15);
  color: white;
  padding: 8px 20px;
  border-radius: 6px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 500;
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
  outline: none;
}

.menu-button:before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 0;
  height: 100%;
  background: rgba(255, 255, 255, 0.1);
  transition: width 0.3s ease;
}

.menu-button:hover {
  border-color: rgba(255, 255, 255, 0.3);
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

.menu-button:hover:before {
  width: 100%;
}

.menu-button:active {
  transform: translateY(1px);
}

.right-section {
  display: flex;
  align-items: center;
  gap: 12px;
}

.time {
  color: white;
  font-size: 14px;
  font-weight: 500;
  padding: 6px 12px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  border: 1px solid rgba(255, 255, 255, 0.15);
  transition: all 0.3s ease;
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 140px;
}

.date {
  font-size: 12px;
  color: rgba(255, 255, 255, 0.8);
}

.time-text {
  font-size: 15px;
  font-weight: 600;
}

.time:hover {
  background: rgba(255, 255, 255, 0.15);
  border-color: rgba(255, 255, 255, 0.25);
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

@media (max-width: 768px) {
  .menu-bar {
    top: 16px;
    left: 16px;
    right: 16px;
    padding: 0 12px;
    height: 50px;
  }

  .menu-button {
    padding: 6px 14px;
    font-size: 13px;
  }

  .time {
    font-size: 13px;
    padding: 6px 12px;
  }

  .logo {
    height: 30px;
    width: 30px;
  }
}

.menu-container {
  background: var(--menu-bg);
  border: 1px solid var(--border-color);
}

.menu-item {
  color: var(--text-color);
}

.menu-item:hover {
  background: var(--menu-hover);
}

.coordinates {
  color: white;
  font-size: 14px;
  font-weight: 500;
  padding: 6px 12px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  margin-right: 12px;
  border: 1px solid rgba(255, 255, 255, 0.15);
}
</style>
