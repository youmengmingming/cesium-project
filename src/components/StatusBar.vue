<template>
  <div class="status-bar">
    <div class="status-content">
      <div class="status-item">
        <i class="fas fa-crosshairs"></i>
        <div class="status-values">
          <ScaleBar :scale="scale" />
        </div>
      </div>
      <div class="status-item">
        <i class="fas fa-crosshairs"></i>
        <div class="status-values">
          <span class="value-label">经纬度:</span>
          <span class="value-text">{{ mousePosition.longitude.toFixed(6) }}°, {{ mousePosition.latitude.toFixed(6) }}°</span>
        </div>
      </div>
      <div class="status-item">
        <i class="fas fa-mountain"></i>
        <div class="status-values">
          <span class="value-label">高度:</span>
          <span class="value-text">{{ cameraInfo.height.toFixed(2) }}米</span>
        </div>
      </div>
      <div class="status-item">
        <i class="fas fa-angle-up"></i>
        <div class="status-values">
          <span class="value-label">俯仰角:</span>
          <span class="value-text">{{ cameraInfo.pitch.toFixed(2) }}°</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, reactive, onMounted, onUnmounted } from 'vue';
import * as Cesium from 'cesium';
import ScaleBar from './ScaleBar.vue';

export default defineComponent({
  name: 'StatusBar',
  components: {
    ScaleBar
  },
  setup() {
    const mousePosition = reactive({
      longitude: 0,
      latitude: 0
    });

    const cameraInfo = reactive({
      height: 0,
      pitch: 0
    });

    const scale = ref('0');

    let viewer: Cesium.Viewer | null = null;
    let handler: Cesium.ScreenSpaceEventHandler | null = null;

    const updateMousePosition = (movement: { endPosition: Cesium.Cartesian2 }) => {
      if (!viewer?.scene) return;
      
      // 获取地表位置
      const position = movement.endPosition;
      const ray = viewer.camera.getPickRay(position);
      if (!ray) return;

      const cartesian = viewer.scene.globe.pick(ray, viewer.scene);
      if (cartesian) {
        const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
        mousePosition.longitude = Cesium.Math.toDegrees(cartographic.longitude);
        mousePosition.latitude = Cesium.Math.toDegrees(cartographic.latitude);
      }
    };

    const calculateScale = (camera: Cesium.Camera) => {
      if (!viewer) return;
      
      const width = viewer.canvas.clientWidth;
      const height = viewer.canvas.clientHeight;
      const center = new Cesium.Cartesian2(width / 2, height / 2);
      
      // 获取屏幕中心点的地表位置
      const centerRay = camera.getPickRay(center);
      if (!centerRay) return;
      
      const centerPosition = viewer.scene.globe.pick(centerRay, viewer.scene);
      if (!centerPosition) return;
      
      // 获取右侧点的位置（固定像素距离）
      const pixelDistance = 100;
      const rightPoint = new Cesium.Cartesian2(width / 2 + pixelDistance, height / 2);
      const rightRay = camera.getPickRay(rightPoint);
      if (!rightRay) return;
      
      const rightPosition = viewer.scene.globe.pick(rightRay, viewer.scene);
      if (!rightPosition) return;

      // 转换为地理坐标
      const centerCartographic = Cesium.Cartographic.fromCartesian(centerPosition);
      const rightCartographic = Cesium.Cartographic.fromCartesian(rightPosition);
      
      // 使用大圆距离计算实际距离
      const geodesic = new Cesium.EllipsoidGeodesic();
      geodesic.setEndPoints(centerCartographic, rightCartographic);
      const realDistance = geodesic.surfaceDistance;

      // 美化后的标准比例尺值（米）
      const standardScales = [
        1, 2, 5, 
        10, 20, 50, 
        100, 200, 500, 
        1000, 2000, 5000, 
        10000, 20000, 50000,
        100000, 200000, 500000,
        1000000
      ];

      // 计算最佳比例尺值
      let bestScale = standardScales[0];
      let bestDiff = Math.abs(realDistance - standardScales[0]);

      for (const scale of standardScales) {
        const diff = Math.abs(realDistance - scale);
        if (diff < bestDiff) {
          bestDiff = diff;
          bestScale = scale;
        }
      }

      // 计算实际显示宽度（像素）
      const displayWidth = (bestScale * pixelDistance) / realDistance;
      
      // 更新比例尺值
      scale.value = bestScale.toString();
      
      // 更新ScaleBar组件的宽度
      const scaleBar = document.querySelector('.scale-line') as HTMLElement;
      if (scaleBar) {
        scaleBar.style.width = `${Math.round(displayWidth)}px`;
      }
    };

    const updateCameraInfo = () => {
      if (!viewer?.camera) return;
      
      const cartographic = viewer.camera.positionCartographic;
      cameraInfo.height = cartographic.height;
      cameraInfo.pitch = Cesium.Math.toDegrees(viewer.camera.pitch);
      
      // 更新比例尺
      calculateScale(viewer.camera);
    };

    let moveHandler: any;
    let cameraHandler: any;

    const initEventHandlers = () => {
      if (!viewer) return;

      // 清理旧的事件处理器
      if (moveHandler) moveHandler();
      if (cameraHandler) cameraHandler();

      // 设置鼠标移动监听
      handler = new Cesium.ScreenSpaceEventHandler(viewer.scene.canvas);
      handler.setInputAction((movement: any) => {
        updateMousePosition(movement);
      }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

      // 设置相机变化监听
      viewer.scene.preRender.addEventListener(updateCameraInfo);
      
      // 立即更新一次信息
      updateCameraInfo();
    };

    onMounted(() => {
      // 等待 viewer 初始化完成
      const checkViewer = setInterval(() => {
        const v = (window as any).cesiumViewer;
        if (v) {
          viewer = v;
          clearInterval(checkViewer);
          initEventHandlers();
          
          // 添加相机移动结束事件监听
          if (viewer) {
            viewer.camera.moveEnd.addEventListener(() => {
              if (viewer) {
                calculateScale(viewer.camera);
              }
            });
          }
        }
      }, 100);
    });

    onUnmounted(() => {
      if (handler) {
        handler.destroy();
      }
      if (viewer?.scene) {
        viewer.scene.preRender.removeEventListener(updateCameraInfo);
      }
      if (viewer?.camera) {
        viewer.camera.moveEnd.removeEventListener(calculateScale);
      }
    });

    return {
      mousePosition,
      cameraInfo,
      scale
    };
  }
});
</script>

<style scoped>
.status-bar {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  display: flex;
  justify-content: center;
  align-items: center;
  font-family: inherit;
  z-index: 1000;
  background-color: transparent;
  /* border-top: 1px solid var(--border-color); */
  /* box-shadow: 0 -2px 10px rgba(0, 0, 0, 0.2); */
  padding: 0;
  height: 40px;
}

.status-content {
  display: flex;
  width: 100%;
  max-width: 1200px;
  justify-content: space-around;
  align-items: center;
  height: 100%;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 0 15px;
  height: 100%;
  border-right: 0px solid var(--border-color);
}

.status-item:last-child {
  border-right: none;
}

.status-item i {
  font-size: 16px;
  color: var(--primary-color);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--transition-normal);
}

.status-values {
  display: flex;
  align-items: center;
  gap: 6px;
}

.value-label {
  font-size: 12px;
  color: var(--text-muted);
  font-weight: 500;
}

.value-text {
  color: var(--text-light);
  font-size: 12px;
  font-weight: 600;
}

.status-item:hover i {
  color: var(--accent-color);
  transform: scale(1.1);
}

@media (max-width: 768px) {
  .status-bar {
    height: 36px;
  }
  
  .status-item {
    padding: 0 8px;
    gap: 4px;
  }
  
  .status-item i {
    font-size: 14px;
  }
  
  .value-label, .value-text {
    font-size: 11px;
  }
}
</style>
