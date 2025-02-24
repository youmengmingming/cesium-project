import * as Cesium from 'cesium';

export class MeasureTool {
  private positions: Cesium.Cartesian3[] = [];
  private polyline: Cesium.Entity | null = null;
  private handler: Cesium.ScreenSpaceEventHandler | null = null;
  private tempPosition: Cesium.Cartesian3 | undefined;
  private labels: Cesium.Entity[] = [];
  private viewer: any;

  constructor(viewer: any) {
    this.viewer = viewer;
  }

  private calculateAzimuth(start: Cesium.Cartesian3, end: Cesium.Cartesian3): number {
    const startCartographic = Cesium.Cartographic.fromCartesian(start);
    const endCartographic = Cesium.Cartographic.fromCartesian(end);
    const geodesic = new Cesium.EllipsoidGeodesic();
    geodesic.setEndPoints(startCartographic, endCartographic);
    const startAzimuth = Cesium.Math.toDegrees(geodesic.startHeading);
    return (startAzimuth + 360) % 360;
  }

  private updateLabel(position: Cesium.Cartesian3, text: string, index: number) {
    if (this.labels[index]) {
      this.viewer.entities.remove(this.labels[index]);
    }
    this.labels[index] = this.viewer.entities.add({
      position,
      label: {
        text,
        font: '14px Microsoft YaHei',
        fillColor: Cesium.Color.fromCssColorString('#00ff00'),
        style: Cesium.LabelStyle.FILL_AND_OUTLINE,
        outlineWidth: 3,
        outlineColor: Cesium.Color.fromCssColorString('#000000'),
        verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
        horizontalOrigin: Cesium.HorizontalOrigin.CENTER,
        disableDepthTestDistance: Number.POSITIVE_INFINITY,
        pixelOffset: new Cesium.Cartesian2(0, -10),
        backgroundColor: Cesium.Color.fromCssColorString('rgba(0,0,0,0.5)'),
        backgroundPadding: new Cesium.Cartesian2(7, 5)
      }
    });
  }

  private calculateDistance(positions: Cesium.Cartesian3[]): number {
    let totalDistance = 0;
    for (let i = 0; i < positions.length - 1; i++) {
      totalDistance += Cesium.Cartesian3.distance(positions[i], positions[i + 1]);
    }
    return totalDistance;
  }

  start() {
    this.handler = new Cesium.ScreenSpaceEventHandler(this.viewer.canvas);
    
    this.handler.setInputAction((event: any) => {
      const earthPosition = this.viewer.scene.pickPosition(event.position);
      if (Cesium.defined(earthPosition)) {
        this.positions.push(earthPosition);
        
        if (this.positions.length === 1) {
          this.polyline = this.viewer.entities.add({
            polyline: {
              positions: new Cesium.CallbackProperty(() => {
                return this.tempPosition ? [...this.positions, this.tempPosition] : this.positions;
              }, false),
              width: 2,
              material: Cesium.Color.YELLOW,
            },
          });
        } else if (this.positions.length > 1) {
          const distance = Cesium.Cartesian3.distance(
            this.positions[this.positions.length - 2],
            this.positions[this.positions.length - 1]
          );
          const azimuth = this.calculateAzimuth(
            this.positions[this.positions.length - 2],
            this.positions[this.positions.length - 1]
          );
          const text = `距离: ${(distance / 1000).toFixed(2)}km\n方位角: ${azimuth.toFixed(1)}°`;
          this.updateLabel(this.positions[this.positions.length - 1], text, this.positions.length - 2);
        }
      }
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK);

    this.handler.setInputAction((event: any) => {
      if (this.positions.length > 0) {
        const newPosition = this.viewer.scene.pickPosition(event.endPosition);
        if (Cesium.defined(newPosition)) {
          this.tempPosition = newPosition;
          if (this.positions.length >= 1) {
            const distance = Cesium.Cartesian3.distance(
              this.positions[this.positions.length - 1],
              newPosition
            );
            const azimuth = this.calculateAzimuth(
              this.positions[this.positions.length - 1],
              newPosition
            );
            const text = `距离: ${(distance / 1000).toFixed(2)}km\n方位角: ${azimuth.toFixed(1)}°`;
            this.updateLabel(newPosition, text, this.positions.length);
          }
        }
      }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

    this.handler.setInputAction(() => {
      if (this.positions.length < 2) return;
      
      const distance = this.calculateDistance(this.positions);
      const kilometers = (distance / 1000).toFixed(2);
      
      const dialog = confirm(`总距离: ${kilometers} 公里\n点击确定关闭测量`);
      if (dialog) {
        this.clear();
      }
    }, Cesium.ScreenSpaceEventType.RIGHT_CLICK);
  }

  clear() {
    if (this.polyline) {
      this.viewer.entities.remove(this.polyline);
    }
    this.labels.forEach(label => {
      this.viewer.entities.remove(label);
    });
    this.labels = [];
    this.positions = [];
    if (this.handler) {
      this.handler.destroy();
      this.handler = null;
    }
    this.tempPosition = undefined;
  }
}
