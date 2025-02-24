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
    <MapSourceSelector class="tool-item" />
  </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import * as Cesium from 'cesium';
import { MeasureTool } from '../utils/measure';
import MapSourceSelector from './MapSourceSelector.vue';

export default defineComponent({
  name: 'ToolBar',
  components: {
    MapSourceSelector
  },
  setup() {
    const measuring = ref(false);
    const marking = ref(false);
    let measureTool: MeasureTool | null = null;

    const toggleMeasure = () => {
      measuring.value = !measuring.value;
      if (measuring.value) {
        measureTool = new MeasureTool(
          (window as any).cesiumViewer,
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

    const toggleMarker = () => {
      marking.value = !marking.value;
      // 标绘功能待实现
    };

    const resetView = () => {
      const viewer = (window as any).cesiumViewer;
      viewer.camera.flyTo({
        destination: Cesium.Cartesian3.fromDegrees(113, 37, 7600000),
        duration: 1.5,
        complete: () => {
          // 可以在这里添加飞行完成后的回调
        }
      });
    };

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
  left: 10px;
  top: 80px; /* 调整顶部距离，避免与菜单栏重叠 */
  background: rgba(28, 32, 38, 0.85);
  border-radius: 4px;
  padding: 5px;
  z-index: 1000;
  display: flex;
  flex-direction: column; /* 使工具按钮垂直排列 */
  gap: 5px; /* 工具按钮之间的间距 */
}

.tool-item {
  padding: 10px;
  color: rgb(255, 255, 255);
  cursor: pointer;
  display: flex;
  flex-direction: column; /* 图标和文字垂直排列 */
  align-items: center;
  text-align: center;
  gap: 5px;
  min-width: 60px; /* 设置最小宽度 */
}

.tool-item span {
  font-size: 12px; /* 调整文字大小 */
}

.tool-item i {
  font-size: 18px; /* 调整图标大小 */
}

.tool-item:hover, .tool-item.active {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
}
</style>
