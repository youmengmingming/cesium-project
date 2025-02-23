<template>
  <div id="app">
    <div class="menu-bar">
      <MenuBar />
    </div>
    <MapView />
    <StatusBar />
  </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import MapView from "./components/Map.vue";
import StatusBar from "./components/StatusBar.vue";
import MenuBar from './components/MenuBar.vue'

export default defineComponent({
  name: "App",
  components: {
    MapView,
    StatusBar,
    MenuBar,
  }
});
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100vh;
  overflow: hidden;
  position: relative;
}

/* 移除底部 logo */
.cesium-viewer-bottom {
  display: none !important;
}

/* 重置基础样式 */
html, body {
  margin: 0;
  padding: 0;
  height: 100%;
  overflow: hidden;
}
</style>
