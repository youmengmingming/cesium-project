<template>
  <div class="map-container">
    <div ref="mapRef" id="cesiumContainer"></div>
    <ToolBar />
  </div>
</template>

<script lang="ts">
import { ref, defineComponent, onMounted } from "vue";
import { Viewer } from "cesium";
import ToolBar from './ToolBar.vue';

export default defineComponent({
  name: "MapView",
  components: {
    ToolBar
  },
  setup() {
    const mapRef = ref<HTMLDivElement | null>(null);

    onMounted(() => {
      if (mapRef.value) {
        const viewer = new Viewer(mapRef.value, {
          shouldAnimate: true,
          animation: false,
          timeline: false,
          fullscreenButton: false,
          geocoder: false,
          homeButton: false,
          sceneModePicker: false,
          navigationHelpButton: false,
          baseLayerPicker: false,
          infoBox: false,
          selectionIndicator: false,
          navigationInstructionsInitiallyVisible: false,
          sceneMode: 3,
          contextOptions: {
            webgl: {
              alpha: true,
            },
          },
        });
        // 将 viewer 实例设置为全局可访问
        (window as any).cesiumViewer = viewer;
      }
    });

    return { mapRef };
  },
});
</script>

<style>
.map-container {
  width: 100%;
  height: 100vh;
  position: relative;
}

#cesiumContainer {
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  left: 0;
}

/* 移除可能影响地图显示的样式 */
.cesium-viewer,
.cesium-widget,
.cesium-widget canvas {
  width: 100%;
  height: 100%;
}

/* 工具栏样式 */
.cesium-viewer-toolbar {
  background: rgba(28, 32, 38, 0.85);
  border: 1px solid rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(8px);
}

.cesium-button {
  color: white;
  background: transparent;
}

.cesium-button:hover {
  background: rgba(255, 255, 255, 0.1) !important;
  color: white !important;
}
</style>
