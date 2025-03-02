import * as Cesium from 'cesium';

// 拖拽状态接口
interface DragState {
  isDragging: boolean;
  lastPosition: Cesium.Cartesian3 | null;
  startScreenPosition: Cesium.Cartesian2 | null;
}

/**
 * 标记工具类 - 用于在地图上添加标记点
 */
export class MarkerTool {
  private viewer: Cesium.Viewer;
  private handler: Cesium.ScreenSpaceEventHandler | null = null;
  private onComplete: (() => void) | null = null;
  private isActive: boolean = false;

  /**
   * 构造函数
   * @param viewer Cesium Viewer实例
   * @param onComplete 完成回调函数
   */
  constructor(viewer: Cesium.Viewer, onComplete?: () => void) {
    this.viewer = viewer;
    this.onComplete = onComplete || null;
  }

  /**
   * 设置拖拽处理
   * @param entity 要拖拽的实体
   * @returns 拖拽处理器
   */
  private setupDragging(entity: Cesium.Entity): Cesium.ScreenSpaceEventHandler {
    const dragState: DragState = {
      isDragging: false,
      lastPosition: null,
      startScreenPosition: null
    };

    const dragHandler = new Cesium.ScreenSpaceEventHandler(this.viewer.scene.canvas);

    // 鼠标悬停效果优化
    dragHandler.setInputAction((movement: any) => {
      const pickedObject = this.viewer.scene.pick(movement.endPosition);
      const isHovering = Cesium.defined(pickedObject) && pickedObject.id === entity;
      
      document.body.style.cursor = isHovering ? 'grab' : 'default';
      
      if (isHovering && !dragState.isDragging) {
        if (entity.billboard) {
          entity.billboard.scale = new Cesium.ConstantProperty(0.6);
          entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.WHITE.withAlpha(1.0));
        }
      } else if (!dragState.isDragging) {
        if (entity.billboard) {
          entity.billboard.scale = new Cesium.ConstantProperty(0.5);
          entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.WHITE.withAlpha(0.9));
        }
      }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

    // 开始拖拽优化
    dragHandler.setInputAction((movement: any) => {
      const pickedObject = this.viewer.scene.pick(movement.position);
      if (Cesium.defined(pickedObject) && pickedObject.id === entity) {
        dragState.isDragging = true;
        dragState.startScreenPosition = movement.position;
        
        const position = entity.position!.getValue(this.viewer.clock.currentTime);
        dragState.lastPosition = position ? position : null;
        
        document.body.style.cursor = 'grabbing';
        this.viewer.scene.screenSpaceCameraController.enableRotate = false;
        
        // 拖拽开始效果
        if (entity.billboard) {
          entity.billboard.scale = new Cesium.ConstantProperty(0.7);
          entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.YELLOW.withAlpha(1.0));
        }
      }
    }, Cesium.ScreenSpaceEventType.LEFT_DOWN);

    // 拖拽移动优化
    dragHandler.setInputAction((movement: any) => {
      if (dragState.isDragging) {
        const scene = this.viewer.scene;
        const ray = scene.camera.getPickRay(movement.endPosition);
        if (!ray) return;
        
        const cartesian = scene.globe.pick(ray, scene);

        if (cartesian) {
          // 限制拖拽边界
          const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
          const longitude = Cesium.Math.toDegrees(cartographic.longitude);
          const latitude = Cesium.Math.toDegrees(cartographic.latitude);
          
          // 设置拖拽边界限制
          if (longitude >= -180 && longitude <= 180 && latitude >= -85 && latitude <= 85) {
            entity.position = new Cesium.ConstantPositionProperty(cartesian);
            
            // 发送坐标更新事件
            window.dispatchEvent(new CustomEvent('updateCoordinates', {
              detail: { longitude, latitude }
            }));

            // 平滑过渡效果
            if (entity.billboard) {
              entity.billboard.scale = new Cesium.ConstantProperty(0.7);
            }
          }
        }
      }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

    // 结束拖拽优化
    dragHandler.setInputAction(() => {
      if (dragState.isDragging) {
        dragState.isDragging = false;
        dragState.lastPosition = null;
        dragState.startScreenPosition = null;
        
        this.viewer.scene.screenSpaceCameraController.enableRotate = true;
        document.body.style.cursor = 'grab';
        
        // 拖拽结束动画效果
        if (entity.billboard) {
          entity.billboard.scale = new Cesium.ConstantProperty(0.6);
          entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.WHITE.withAlpha(1.0));
        }
        
        setTimeout(() => {
          if (!dragState.isDragging && entity.billboard) {
            entity.billboard.scale = new Cesium.ConstantProperty(0.5);
            entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.WHITE.withAlpha(0.9));
          }
        }, 200);
      }
    }, Cesium.ScreenSpaceEventType.LEFT_UP);

    // 双击定位优化
    dragHandler.setInputAction((movement: any) => {
      const pickedObject = this.viewer.scene.pick(movement.position);
      if (Cesium.defined(pickedObject) && pickedObject.id === entity) {
        const position = entity.position!.getValue(this.viewer.clock.currentTime);
        if (!position) return;
        
        const cartographic = Cesium.Cartographic.fromCartesian(position);
        
        // 优化相机飞行动画
        this.viewer.camera.flyTo({
          destination: Cesium.Cartesian3.fromRadians(
            cartographic.longitude,
            cartographic.latitude,
            cartographic.height + 2000
          ),
          orientation: {
            heading: Cesium.Math.toRadians(0),
            pitch: Cesium.Math.toRadians(-50),
            roll: 0
          },
          duration: 1.5,
          complete: () => {
            // 飞行完成后的视觉反馈
            if (entity.billboard) {
              entity.billboard.scale = new Cesium.ConstantProperty(0.7);
              entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.YELLOW.withAlpha(1.0));
              
              setTimeout(() => {
                if (entity.billboard) {
                  entity.billboard.scale = new Cesium.ConstantProperty(0.5);
                  entity.billboard.color = new Cesium.ConstantProperty(Cesium.Color.WHITE.withAlpha(0.9));
                }
              }, 300);
            }
          }
        });
      }
    }, Cesium.ScreenSpaceEventType.LEFT_DOUBLE_CLICK);

    return dragHandler;
  }

  /**
   * 开始标记
   */
  start(): void {
    if (this.isActive) return;
    this.isActive = true;
    
    this.handler = new Cesium.ScreenSpaceEventHandler(this.viewer.scene.canvas);
    
    this.handler.setInputAction((click: any) => {
      const cartesian = this.viewer.camera.pickEllipsoid(
        click.position,
        this.viewer.scene.globe.ellipsoid
      );
      
      if (cartesian) {
        const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
        const longitude = Cesium.Math.toDegrees(cartographic.longitude);
        const latitude = Cesium.Math.toDegrees(cartographic.latitude);

        const entity = this.viewer.entities.add({
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
        this.setupDragging(entity);

        // 发送坐标更新事件
        window.dispatchEvent(new CustomEvent('updateCoordinates', {
          detail: { longitude, latitude }
        }));
        
        this.stop();
        
        if (this.onComplete) {
          this.onComplete();
        }
      }
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK);
  }

  /**
   * 停止标记
   */
  stop(): void {
    if (this.handler) {
      this.handler.destroy();
      this.handler = null;
    }
    this.isActive = false;
  }

  /**
   * 获取活动状态
   */
  getIsActive(): boolean {
    return this.isActive;
  }
}

/**
 * 视图工具类 - 用于视图操作
 */
export class ViewTool {
  private viewer: Cesium.Viewer;

  /**
   * 构造函数
   * @param viewer Cesium Viewer实例
   */
  constructor(viewer: Cesium.Viewer) {
    this.viewer = viewer;
  }

  /**
   * 重置视图到默认位置
   * @param callback 完成回调函数
   */
  resetView(callback?: () => void): void {
    this.viewer.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(113, 37, 7600000),
      duration: 1.5,
      complete: callback
    });
  }

  /**
   * 飞行到指定位置
   * @param longitude 经度
   * @param latitude 纬度
   * @param height 高度
   * @param duration 飞行时间（秒）
   * @param callback 完成回调函数
   */
  flyTo(longitude: number, latitude: number, height: number, duration: number = 1.5, callback?: () => void): void {
    this.viewer.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(longitude, latitude, height),
      duration: duration,
      complete: callback
    });
  }
} 