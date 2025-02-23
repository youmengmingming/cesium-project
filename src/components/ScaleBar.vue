<template>
  <div class="scale-bar">
    <div class="scale-text">{{ formatScale(scale) }}</div>
    <div class="scale-line">
      <div class="line-start"></div>
      <div class="line-main"></div>
      <div class="line-end"></div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'ScaleBar',
  props: {
    scale: {
      type: String,
      required: true
    }
  },
  methods: {
    formatScale(scale: string) {
      const value = parseInt(scale)
      if (value >= 1000) {
        return `${(value / 1000).toFixed(1)} 公里`
      }
      return `${value} 米`
    }
  }
})
</script>

<style scoped>
.scale-bar {
  background: rgba(28, 32, 38, 0.85);
  padding: 8px 12px;
  border-radius: 6px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
  backdrop-filter: blur(8px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  display: flex;
  flex-direction: column;
  gap: 4px;
  align-items: center;
}

.scale-text {
  color: white;
  font-size: 12px;
  font-weight: 500;
  letter-spacing: 0.3px;
}

.scale-line {
  display: flex;
  align-items: center;
  width: 100px;  /* 固定宽度 */
  height: 8px;
  position: relative;
}

.line-main {
  height: 2px;
  background: white;
  flex-grow: 1;
}

.line-start, .line-end {
  width: 2px;
  height: 8px;
  background: white;
}

.scale-bar:hover {
  background: rgba(28, 32, 38, 0.95);
  border-color: rgba(255, 255, 255, 0.2);
}

@media (max-width: 768px) {
  .scale-bar {
    padding: 6px 10px;
  }
  
  .scale-text {
    font-size: 11px;
  }
  
  .scale-line {
    width: 80px;
  }
}
</style>
