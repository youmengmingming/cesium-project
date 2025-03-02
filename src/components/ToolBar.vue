<template>
  <div class="toolbar">
    <div class="tool-item" @click="toggleMeasure" :class="{ active: measuring }">
      <i class="fas fa-ruler"></i>
      <span>测距</span>
    </div>
    <div class="tool-item" @click="toggleMarker" :class="{ active: marking }">
      <i class="fas fa-map-marker-alt"></i>
      <span>标绘</span>
    </div>
    <div class="tool-item" @click="resetView">
      <i class="fas fa-home"></i>
      <span>复位</span>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onUnmounted } from 'vue';
import * as Cesium from 'cesium';
import { MeasureTool } from '../utils/measure';
import { MarkerTool, ViewTool } from '../utils/mapTools';

export default defineComponent({
  name: 'ToolBar',
  setup() {
    const measuring = ref(false);
    const marking = ref(false);
    let measureTool: MeasureTool | null = null;
    let markerTool: MarkerTool | null = null;
    let viewTool: ViewTool | null = null;

    // 初始化工具
    const initTools = () => {
      const viewer = (window as any).cesiumViewer;
      if (viewer) {
        if (!markerTool) {
          markerTool = new MarkerTool(viewer, () => {
            marking.value = false;
          });
        }
        if (!viewTool) {
          viewTool = new ViewTool(viewer);
        }
      }
    };

    // 测距工具
    const toggleMeasure = () => {
      measuring.value = !measuring.value;
      const viewer = (window as any).cesiumViewer;
      
      if (measuring.value) {
        measureTool = new MeasureTool(
          viewer,
          () => {
            measuring.value = false;
            measureTool = null;
          }
        );
        measureTool.start();
      } else if (measureTool) {
        measureTool.clear();
        measureTool = null;
      }
    };

    // 标记工具
    const toggleMarker = () => {
      initTools();
      
      if (!markerTool) return;
      
      marking.value = !marking.value;
      
      if (marking.value) {
        markerTool.start();
      } else {
        markerTool.stop();
      }
    };

    // 重置视图
    const resetView = () => {
      initTools();
      
      if (!viewTool) return;
      
      viewTool.resetView();
    };

    // 组件卸载时清理资源
    onUnmounted(() => {
      if (measureTool) {
        measureTool.clear();
        measureTool = null;
      }
      
      if (markerTool) {
        markerTool.stop();
        markerTool = null;
      }
    });

    return {
      measuring,
      marking,
      toggleMeasure,
      toggleMarker,
      resetView,
    };
  },
});
</script>

<style scoped>
.toolbar {
  position: absolute;
  left: 16px;
  top: 96px; /* 调整顶部距离，避免与菜单栏重叠 */
  background: var(--bg-dark);
  border-radius: var(--radius-md);
  padding: 10px;
  z-index: 1000;
  display: flex;
  flex-direction: column;
  gap: 8px;
  box-shadow: var(--shadow-md);
  border: 1px solid var(--border-color);
  backdrop-filter: blur(12px);
}

.tool-item {
  padding: 12px;
  color: var(--text-light);
  cursor: pointer;
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  gap: 6px;
  min-width: 64px;
  border-radius: var(--radius-sm);
  transition: all var(--transition-normal);
  border: 1px solid transparent;
}

.tool-item span {
  font-size: 12px;
  font-weight: 500;
}

.tool-item i {
  font-size: 18px;
  color: var(--primary-color);
  transition: all var(--transition-normal);
}

.tool-item:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: var(--border-hover);
  transform: translateY(-2px);
  box-shadow: var(--shadow-sm);
}

.tool-item:hover i {
  color: var(--accent-color);
  transform: scale(1.1);
}

.tool-item.active {
  background: rgba(var(--primary-color-rgb), 0.2);
  border-color: var(--primary-color);
}

.tool-item.active i {
  color: var(--primary-color);
}

@media (max-width: 768px) {
  .toolbar {
    left: 12px;
    top: 80px;
    padding: 8px;
  }
  
  .tool-item {
    padding: 10px;
    min-width: 56px;
  }
  
  .tool-item i {
    font-size: 16px;
  }
  
  .tool-item span {
    font-size: 11px;
  }
}
</style>
