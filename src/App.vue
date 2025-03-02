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
:root {
  --primary-color: #3498db;
  --primary-dark: #2980b9;
  --primary-color-rgb: 52, 152, 219;
  --accent-color: #2ecc71;
  --accent-dark: #27ae60;
  --warning-color: #f39c12;
  --danger-color: #e74c3c;
  --bg-dark: rgba(22, 26, 32, 0.9);
  --bg-light: rgba(32, 36, 42, 0.85);
  --text-light: #ffffff;
  --text-muted: rgba(255, 255, 255, 0.7);
  --border-color: rgba(255, 255, 255, 0.15);
  --border-hover: rgba(255, 255, 255, 0.3);
  --shadow-sm: 0 2px 8px rgba(0, 0, 0, 0.15);
  --shadow-md: 0 4px 12px rgba(0, 0, 0, 0.2);
  --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.25);
  --radius-sm: 4px;
  --radius-md: 8px;
  --radius-lg: 12px;
  --transition-fast: 0.2s ease;
  --transition-normal: 0.3s ease;
}

#app {
  font-family: 'SF Pro Display', 'Segoe UI', 'Roboto', 'Helvetica Neue', Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100vh;
  overflow: hidden;
  position: relative;
  color: var(--text-light);
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

/* 全局按钮样式 */
button {
  font-family: inherit;
  cursor: pointer;
  transition: var(--transition-normal);
}

/* 全局滚动条样式 */
::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}

::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.1);
  border-radius: 4px;
}

::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.2);
  border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
  background: rgba(255, 255, 255, 0.3);
}
</style>
