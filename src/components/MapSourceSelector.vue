<template>
  <div class="map-source-selector" @click="cycleDataSource">
    <i class="fas fa-database"></i>
    <span>{{ dataSourceLabels[currentDataSource] }}</span>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import * as Cesium from 'cesium';
import { imageryProviders, DataSourceType, dataSourceLabels } from '../config/imageryConfig';

export default defineComponent({
  name: 'MapSourceSelector',
  setup() {
    const currentDataSource = ref<DataSourceType>(DataSourceType.DEFAULT);

    const cycleDataSource = () => {
      const sources = [
        DataSourceType.DEFAULT,
        DataSourceType.OSM,
        DataSourceType.AMAP,
        DataSourceType.AMAP_LABEL,
        DataSourceType.OFFLINE_LOCAL,
        DataSourceType.OFFLINE_SERVER
      ];
      
      const currentIndex = sources.indexOf(currentDataSource.value);
      const nextIndex = (currentIndex + 1) % sources.length;
      currentDataSource.value = sources[nextIndex];
      
      const viewer = (window as any).cesiumViewer;
      const layers = viewer.imageryLayers;
      layers.removeAll();
      
      if (currentDataSource.value === DataSourceType.OFFLINE_LOCAL) {
        layers.addImageryProvider(imageryProviders.offline.localTiles);
      } else if (currentDataSource.value === DataSourceType.OFFLINE_SERVER) {
        layers.addImageryProvider(imageryProviders.offline.localServer);
      } else {
        const provider = imageryProviders.online[currentDataSource.value];
        layers.addImageryProvider(provider);
        
        if (currentDataSource.value === DataSourceType.AMAP) {
          layers.addImageryProvider(imageryProviders.online.amapLabel);
        }
      }
    };

    return {
      currentDataSource,
      dataSourceLabels,
      cycleDataSource,
    };
  }
});
</script>

<style scoped>
.map-source-selector {
  padding: 10px;
  color: rgb(255, 255, 255);
  cursor: pointer;
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  gap: 5px;
  min-width: 60px;
}

.map-source-selector:hover {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
}

.map-source-selector span {
  font-size: 12px;
}

.map-source-selector i {
  font-size: 18px;
}
</style>
