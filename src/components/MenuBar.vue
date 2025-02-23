<template>
  <div class="menu-bar">
    <div class="left-section">
      <div class="logo">
        <img src="../assets/logo.png" alt="Logo">
      </div>
      <div class="buttons">
        <button v-for="i in 4" :key="i" class="menu-button">
          测试{{ i }}
        </button>
      </div>
    </div>
    <div class="right-section">
      <div class="time">
        <div class="date">{{ currentDate }}</div>
        <div class="time-text">{{ currentTime }}</div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onMounted, onUnmounted } from 'vue'

export default defineComponent({
  name: 'MenuBar',
  setup() {
    const currentTime = ref('')
    const currentDate = ref('')
    let timer: number

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
    })

    onUnmounted(() => {
      clearInterval(timer)
    })

    return {
      currentTime,
      currentDate
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
</style>
