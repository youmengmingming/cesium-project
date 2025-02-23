<template>
  <div class="toolbar">
    <div class="tool-item" @click="startMeasure" :class="{ active: measuring }">
      <i class="fas fa-ruler"></i>
      <span>测距</span>
    </div>
    <div class="tool-item" @click="Marker" :class="{ active: measuring }">
      <i class="fas fa-ruler"></i>
      <span>标绘</span>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import * as Cesium from 'cesium';

export default defineComponent({
  name: 'ToolBar',
  setup() {
    const measuring = ref(false);
    let positions: Cesium.Cartesian3[] = [];
    let polyline: Cesium.Entity | null = null;
    let handler: Cesium.ScreenSpaceEventHandler | null = null;
    let tempPosition: Cesium.Cartesian3 | undefined;
    let labels: Cesium.Entity[] = []; // 存储标签实体

    const clearMeasurement = () => {
      if (polyline) {
        (window as any).cesiumViewer.entities.remove(polyline);
      }
      // 清除所有标签
      labels.forEach(label => {
        (window as any).cesiumViewer.entities.remove(label);
      });
      labels = [];
      positions = [];
      if (handler) {
        handler.destroy();
        handler = null;
      }
      measuring.value = false;
      tempPosition = undefined;
    };

    // 计算方位角（与正北方向的夹角）
    const calculateAzimuth = (start: Cesium.Cartesian3, end: Cesium.Cartesian3): number => {
      const startCartographic = Cesium.Cartographic.fromCartesian(start);
      const endCartographic = Cesium.Cartographic.fromCartesian(end);
      const geodesic = new Cesium.EllipsoidGeodesic();
      geodesic.setEndPoints(startCartographic, endCartographic);
      const startAzimuth = Cesium.Math.toDegrees(geodesic.startHeading);
      return (startAzimuth + 360) % 360;
    };

    // 添加或更新标签
    const updateLabel = (position: Cesium.Cartesian3, text: string, index: number) => {
      const viewer = (window as any).cesiumViewer;
      if (labels[index]) {
        viewer.entities.remove(labels[index]);
      }
      labels[index] = viewer.entities.add({
        position,
        label: {
          text,
          font: '14px Microsoft YaHei',
          fillColor: Cesium.Color.fromCssColorString('#00ff00'),  // 修改为荧光绿色
          style: Cesium.LabelStyle.FILL_AND_OUTLINE,
          outlineWidth: 3,
          outlineColor: Cesium.Color.fromCssColorString('#000000'),  // 黑色描边
          verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
          horizontalOrigin: Cesium.HorizontalOrigin.CENTER,
          disableDepthTestDistance: Number.POSITIVE_INFINITY,
          pixelOffset: new Cesium.Cartesian2(0, -10),
          backgroundColor: Cesium.Color.fromCssColorString('rgba(0,0,0,0.5)'),  // 半透明黑色背景
          backgroundPadding: new Cesium.Cartesian2(7, 5)  // 背景内边距
        }
      });
    };

    const calculateDistance = (positions: Cesium.Cartesian3[]): number => {
      let totalDistance = 0;
      for (let i = 0; i < positions.length - 1; i++) {
        totalDistance += Cesium.Cartesian3.distance(positions[i], positions[i + 1]);
      }
      return totalDistance;
    };

    const Marker = () => {
        
        return {
      Marker,
    };
    };

    const startMeasure = () => {
      const viewer = (window as any).cesiumViewer;
      measuring.value = !measuring.value;

      if (!measuring.value) {
        clearMeasurement();
        return;
      }

      handler = new Cesium.ScreenSpaceEventHandler(viewer.canvas);
      
      handler.setInputAction((event: any) => {
        const earthPosition = viewer.scene.pickPosition(event.position);
        if (Cesium.defined(earthPosition)) {
          positions.push(earthPosition);
          
          if (positions.length === 1) {
            polyline = viewer.entities.add({
              polyline: {
                positions: new Cesium.CallbackProperty(() => {
                  return tempPosition ? [...positions, tempPosition] : positions;
                }, false),
                width: 2,
                material: Cesium.Color.YELLOW,
              },
            });
          } else if (positions.length > 1) {
            const distance = Cesium.Cartesian3.distance(
              positions[positions.length - 2],
              positions[positions.length - 1]
            );
            const azimuth = calculateAzimuth(
              positions[positions.length - 2],
              positions[positions.length - 1]
            );
            const text = `距离: ${(distance / 1000).toFixed(2)}km\n方位角: ${azimuth.toFixed(1)}°`;
            updateLabel(positions[positions.length - 1], text, positions.length - 2);
          }
        }
      }, Cesium.ScreenSpaceEventType.LEFT_CLICK);

      handler.setInputAction((event: any) => {
        if (positions.length > 0) {
          const newPosition = viewer.scene.pickPosition(event.endPosition);
          if (Cesium.defined(newPosition)) {
            tempPosition = newPosition;
            if (positions.length >= 1) {
              const distance = Cesium.Cartesian3.distance(
                positions[positions.length - 1],
                newPosition
              );
              const azimuth = calculateAzimuth(
                positions[positions.length - 1],
                newPosition
              );
              const text = `距离: ${(distance / 1000).toFixed(2)}km\n方位角: ${azimuth.toFixed(1)}°`;
              updateLabel(newPosition, text, positions.length);
            }
          }
        }
      }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

      handler.setInputAction(() => {
        if (positions.length < 2) return;
        
        const distance = calculateDistance(positions);
        const kilometers = (distance / 1000).toFixed(2);
        
        const dialog = confirm(`总距离: ${kilometers} 公里\n点击确定关闭测量`);
        if (dialog) {
          clearMeasurement();
        }
      }, Cesium.ScreenSpaceEventType.RIGHT_CLICK);
    };

    return {
      measuring,
      startMeasure,
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
