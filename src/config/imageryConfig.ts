import * as Cesium from 'cesium';

export const imageryProviders = {
  online: {
    default: new Cesium.IonImageryProvider({
      assetId: Cesium.IonImageryProvider.BING_AERIAL
    }),
    osm: new Cesium.OpenStreetMapImageryProvider({
      url: 'https://tile.openstreetmap.org/'
    }),
  
    // 高德地图
    amap: new Cesium.UrlTemplateImageryProvider({
      url: 'https://webst{s}.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}',
      subdomains: ['01', '02', '03', '04'],
      minimumLevel: 1,
      maximumLevel: 18,
      credit: '© 高德地图'
    }),
    amapLabel: new Cesium.UrlTemplateImageryProvider({
      url: 'https://webst{s}.is.autonavi.com/appmaptile?style=8&x={x}&y={y}&z={z}',
      subdomains: ['01', '02', '03', '04'],
      minimumLevel: 1,
      maximumLevel: 18
    }),
  },
  offline: {
    localTiles: new Cesium.UrlTemplateImageryProvider({
      url: '/tiles/{z}/{x}/{y}.png', // 离线瓦片路径
      tilingScheme: new Cesium.WebMercatorTilingScheme(),
      minimumLevel: 1,
      maximumLevel: 20
    }),
    localServer: new Cesium.UrlTemplateImageryProvider({
      url: 'http://localhost:8081/tiles/{z}/{x}/{y}.png',  // 本地服务器瓦片地址
      tilingScheme: new Cesium.WebMercatorTilingScheme(),
      minimumLevel: 1,
      maximumLevel: 18,
      credit: '本地服务器影像'
    })
  }
};

export enum DataSourceType {
  DEFAULT = 'default',
  OSM = 'osm',
  OFFLINE_LOCAL = 'offline_local',
  OFFLINE_SERVER = 'offline_server',
  AMAP = 'amap',
  AMAP_LABEL = 'amapLabel',
}

export const dataSourceLabels = {
  [DataSourceType.DEFAULT]: '默认',
  [DataSourceType.OSM]: 'OpenStreetMap',
  [DataSourceType.OFFLINE_LOCAL]: '本地离线',
  [DataSourceType.OFFLINE_SERVER]: '本地服务器',
  [DataSourceType.AMAP]: '高德地图',
  [DataSourceType.AMAP_LABEL]: '高德带标注',
};
