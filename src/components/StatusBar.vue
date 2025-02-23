<template>
  <div class="status-bar">
    <div class="status-group left">
      <ScaleBar :scale="scale" />
    </div>
    <div class="status-group right">
      <div class="status-item">
        <i class="fas fa-crosshairs"></i>
        <div class="status-values">
          <div class="value-label">经纬度</div>
          <div class="value-text">{{ mousePosition.longitude.toFixed(6) }}°, {{ mousePosition.latitude.toFixed(6) }}°</div>
        </div>
      </div>
      <div class="status-item">
        <i class="fas fa-mountain"></i>
        <div class="status-values">
          <div class="value-label">高度</div>
          <div class="value-text">{{ cameraInfo.height.toFixed(2) }}米</div>
        </div>
      </div>
      <div class="status-item">
        <i class="fas fa-angle-up"></i>
        <div class="status-values">
          <div class="value-label">俯仰角</div>
          <div class="value-text">{{ cameraInfo.pitch.toFixed(2) }}°</div>
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
          viewer.camera.moveEnd.addEventListener(() => {
            calculateScale(viewer!.camera);
          });
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
  bottom: 25px;
  left: 25px;
  right: 25px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-family: "SF Pro Display", "Helvetica Neue", Arial, sans-serif;
  z-index: 1000;
}

.status-group {
  display: flex;
  gap: 12px;
}

.status-group.right {
  background: rgba(28, 32, 38, 0.85);
  padding: 8px;
  border-radius: 6px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
  backdrop-filter: blur(8px);
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.status-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 6px 12px;
  border-right: 1px solid rgba(255, 255, 255, 0.15);
  min-width: 180px;
}

.status-item:last-child {
  border-right: none;
}

.status-item i {
  font-size: 16px;
  color: #64B5F6;
  width: 20px;
  height: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.status-values {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.value-label {
  font-size: 11px;
  color: rgba(255, 255, 255, 0.6);
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.value-text {
  color: white;
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.3px;
}

.status-item:hover {
  background: rgba(255, 255, 255, 0.08);
  border-radius: 4px;
}

.status-item:hover i {
  color: #90CAF9;
  transform: scale(1.1);
}

.status-item {
  transition: all 0.2s ease;
}

.status-item i {
  transition: all 0.2s ease;
}

@media (max-width: 768px) {
  .status-item {
    min-width: 140px;
  }
  
  .value-text {
    font-size: 12px;
  }
}
</style>
