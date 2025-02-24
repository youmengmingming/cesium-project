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
    let handler: Cesium.ScreenSpaceEventHandler | null = null;

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

    // 添加拖拽处理方法
    const setupDragging = (viewer: any, entity: any) => {
      const dragHandler = new Cesium.ScreenSpaceEventHandler(viewer.scene.canvas);
      let isDragging = false;

      // 鼠标悬停效果
      dragHandler.setInputAction((e: any) => {
        const pickedObject = viewer.scene.pick(e.endPosition);
        if (Cesium.defined(pickedObject) && pickedObject.id === entity) {
          document.body.style.cursor = 'move';
          entity.billboard.scale = entity.billboard.scale.getValue() * 1.1;
        } else {
          document.body.style.cursor = 'default';
          if (!isDragging) {
            entity.billboard.scale = entity.billboard.scale.getValue() / 1.1;
          }
        }
      }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

      // 开始拖拽
      dragHandler.setInputAction((e: any) => {
        const pickedObject = viewer.scene.pick(e.position);
        if (Cesium.defined(pickedObject) && pickedObject.id === entity) {
          isDragging = true;
          viewer.scene.screenSpaceCameraController.enableRotate = false;
          entity.billboard.scale = entity.billboard.scale.getValue() * 1.2;
        }
      }, Cesium.ScreenSpaceEventType.LEFT_DOWN);

      // 拖拽移动
      dragHandler.setInputAction((e: any) => {
        if (isDragging) {
          const cartesian = viewer.scene.camera.pickEllipsoid(
            e.endPosition,
            viewer.scene.globe.ellipsoid
          );
          if (cartesian) {
            entity.position = cartesian;
            
            // 获取新坐标并发送更新事件
            const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
            const newLongitude = Cesium.Math.toDegrees(cartographic.longitude);
            const newLatitude = Cesium.Math.toDegrees(cartographic.latitude);
            
            window.dispatchEvent(new CustomEvent('updateCoordinates', {
              detail: { longitude: newLongitude, latitude: newLatitude }
            }));
          }
        }
      }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

      // 结束拖拽
      dragHandler.setInputAction(() => {
        if (isDragging) {
          isDragging = false;
          viewer.scene.screenSpaceCameraController.enableRotate = true;
          entity.billboard.scale = entity.billboard.scale.getValue() / 1.2;
          document.body.style.cursor = 'default';
        }
      }, Cesium.ScreenSpaceEventType.LEFT_UP);

      // 双击定位
      dragHandler.setInputAction((e: any) => {
        const pickedObject = viewer.scene.pick(e.position);
        if (Cesium.defined(pickedObject) && pickedObject.id === entity) {
          const cartesian = entity.position.getValue(viewer.clock.currentTime);
          const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
          
          viewer.camera.flyTo({
            destination: Cesium.Cartesian3.fromRadians(
              cartographic.longitude,
              cartographic.latitude,
              cartographic.height + 1000
            ),
            orientation: {
              heading: 0.0,
              pitch: Cesium.Math.toRadians(-45),
              roll: 0.0
            },
            duration: 1.5,
            complete: () => {
              // 完成飞行后的回调
              entity.billboard.scale = entity.billboard.scale.getValue() * 1.2;
              setTimeout(() => {
                entity.billboard.scale = entity.billboard.scale.getValue() / 1.2;
              }, 200);
            }
          });
        }
      }, Cesium.ScreenSpaceEventType.LEFT_DOUBLE_CLICK);

      return dragHandler;
    };

    const toggleMarker = () => {
      marking.value = !marking.value;
      const viewer = (window as any).cesiumViewer;
      
      if (marking.value) {
        handler = new Cesium.ScreenSpaceEventHandler(viewer.scene.canvas);
        
        handler.setInputAction((click: any) => {
          const cartesian = viewer.camera.pickEllipsoid(
            click.position,
            viewer.scene.globe.ellipsoid
          );
          
          if (cartesian) {
            const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
            const longitude = Cesium.Math.toDegrees(cartographic.longitude);
            const latitude = Cesium.Math.toDegrees(cartographic.latitude);

            const entity = viewer.entities.add({
              position: Cesium.Cartesian3.fromDegrees(longitude, latitude),
              billboard: {
                image: new URL('../assets/logo.png', import.meta.url).href,
                verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
                horizontalOrigin: Cesium.HorizontalOrigin.CENTER,
                scale: 0.5,
                heightReference: Cesium.HeightReference.CLAMP_TO_GROUND,
                scaleByDistance: new Cesium.NearFarScalar(1000, 1.0, 5000000, 0.3),
                translucencyByDistance: new Cesium.NearFarScalar(1000, 1.0, 5000000, 0.5)
              }
            });

            // 设置拖拽处理
            setupDragging(viewer, entity);

            window.dispatchEvent(new CustomEvent('updateCoordinates', {
              detail: { longitude, latitude }
            }));
            
            marking.value = false;
            if (handler) {
              handler.destroy();
              handler = null;
            }
          }
        }, Cesium.ScreenSpaceEventType.LEFT_CLICK);
      } else if (handler) {
        handler.destroy();
        handler = null;
      }
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
