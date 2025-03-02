<template>
  <div class="toolbar">
    <div class="tool-item" @click="toggleMeasure" :class="{ active: measuring }">
      <svg t="1740929387071" class="icon" viewBox="0 0 1024 1024" version="1.1" xmlns="http://www.w3.org/2000/svg" p-id="8902" width="24" height="24"><path d="M32.525474 719.683368a26.947368 26.947368 0 0 1 0-38.103579L680.394105 33.684211a26.947368 26.947368 0 0 1 38.103579 0l270.551579 270.578526a26.947368 26.947368 0 0 1 0 38.103579l-647.814737 647.868631a26.947368 26.947368 0 0 1-38.130526 0L32.525474 719.683368z m289.630315 137.189053L855.686737 323.368421l-156.240842-156.240842-43.843369 43.816421 95.285895 95.285895-34.304 34.304L621.325474 245.221053l-34.304 34.304 60.981894 60.981894-34.304 34.277053-60.981894-60.954947-34.304 34.277052 60.981894 60.981895-34.304 34.304-60.954947-60.981895-34.304 34.304 95.258947 95.258948-34.277052 34.304-95.285895-95.258948-34.304 34.304 60.981895 60.954948-34.304 34.304-60.981895-60.981895-34.304 34.304 60.981895 60.981895-34.304 34.304-60.954948-60.981895L244.035368 622.484211l95.285895 95.285894-34.304 34.304-95.285895-95.285894L165.914947 700.631579l156.240842 156.240842z" fill="#ffffff" fill-opacity=".85" p-id="8903"></path></svg>
    </div>
    <div class="tool-item" @click="toggleMarker" :class="{ active: marking }">
      <svg t="1740929350279" class="icon" viewBox="0 0 1024 1024" version="1.1" xmlns="http://www.w3.org/2000/svg" p-id="7891" width="24" height="24"><path d="M819.2 870.4a51.2 51.2 0 0 1 51.2-51.2V204.8A51.2 51.2 0 0 1 819.2 153.6H204.8A51.2 51.2 0 0 1 153.6 204.8v281.6a25.6 25.6 0 0 1-25.6 25.6h-51.2a25.6 25.6 0 0 1-25.6-25.6V204.8A51.2 51.2 0 0 1 0 153.6v-102.4A51.2 51.2 0 0 1 51.2 0h102.4A51.2 51.2 0 0 1 204.8 51.2h614.4A51.2 51.2 0 0 1 870.4 0h102.4A51.2 51.2 0 0 1 1024 51.2v102.4A51.2 51.2 0 0 1 972.8 204.8v614.4a51.2 51.2 0 0 1 51.2 51.2v102.4a51.2 51.2 0 0 1-51.2 51.2h-102.4a51.2 51.2 0 0 1-51.2-51.2h-230.4a25.6 25.6 0 0 1-25.6-25.6v-51.2a25.6 25.6 0 0 1 25.6-25.6H819.2z" p-id="7892" fill="#ffffff"></path><path d="M730.0096 373.179733l-62.122667-62.122666c-33.928533-33.928533-90.385067-33.928533-129.9456 0L108.6464 740.352l192.034133 192.068267 429.329067-429.329067c39.5264-39.5264 39.5264-96.017067 0-129.911467z m-124.279467 186.402134c-11.332267 11.332267-33.928533 11.332267-45.192533 0l-73.4208-67.754667c-11.264-11.298133-11.264-33.8944 0-45.192533 11.298133-11.298133 33.928533-11.298133 45.1584 0l67.7888 67.822933c16.930133 11.229867 16.930133 33.826133 5.666133 45.124267zM52.155733 960.682667c-5.666133 28.2624 16.930133 45.192533 39.5264 39.5264l146.875734-39.5264-158.173867-158.173867-28.228267 158.173867z" p-id="7893" fill="#ffffff"></path></svg>
    </div>
    <div class="tool-item" @click="resetView">
      <svg t="1740929061481" class="icon" viewBox="0 0 1024 1024" version="1.1" xmlns="http://www.w3.org/2000/svg" p-id="5605" width="24" height="24"><path d="M1023.968793 512.0972c0-282.258242-228.984556-511.073966-511.452563-511.073966-282.473122 0-511.462795 228.815722-511.462795 511.073966 0 260.737573 195.390741 475.854231 447.837044 507.156 14.231147 2.999101 31.564741 4.746785 52.209522 4.746785 5.795601 0 11.384509-0.304924 16.78207-0.857471C797.877943 1020.265179 1023.968793 792.575015 1023.968793 512.0972zM950.836169 519.328399c-1.471411-25.500029-4.8399-50.460814-9.969375-74.718637 0.455339-8.661681 0.579151-16.868022 0.346877-24.49726 6.343032 29.660502 9.687986 60.432236 9.687986 91.984698C950.902679 514.513057 950.875051 516.921751 950.836169 519.328399zM74.120572 512.0972c0-30.977405 3.225236-61.200685 9.345202-90.360825 18.931886 20.247766 63.554125 19.769915 74.436224-10.483039 19.471131 11.604504 45.637286 13.717484 45.637286 36.912165 0 76.550227 2.725897 158.618759 72.279247 159.886547 1.958471 0.025581 38.788778 13.957944 56.316786 59.416164 6.060619 15.711767 30.032959 0 56.321902 0 13.124008 0 0 22.110053 0 69.921714 0 47.626454 102.680569 120.959633 102.680569 120.959633-0.475804 31.525858 0.818588 57.017702 3.442161 77.38314-23.177287-0.426689-42.709813 2.645062-58.055262 7.868674C230.627375 907.644909 74.120572 728.142955 74.120572 512.0972zM620.440877 936.783561c-2.272604-11.126653-12.215375-17.220015-30.356301-12.451742 14.474677-61.640676 21.511461-96.169728 51.728602-122.387045 43.719746-37.898564 5.206218-80.04355-28.063232-75.076769-26.222433 3.958895-9.651149-32.467234-33.055594-34.481983-23.404445-1.958471-53.969486-48.511552-87.653346-64.530291-17.855443-8.479545-35.402894-31.203539-62.940184-32.221658-24.407215-0.946492-60.07615 20.637618-60.07615 3.999824 0-53.590889-5.426213-91.832236-6.541539-107.104012-0.900446-12.269607-8.020113-4.132845 24.974087-3.339838 17.956744 0.48092 9.185577-36.067997 26.959162-37.495409 17.456382-1.381367 59.052916 16.341056 69.650557 9.277668 9.845564-6.577352 72.371338 164.12683 72.371338 28.216717 0-16.126177-8.351641-44.163828 0-59.436629 33.030013-60.352424 63.952163-109.539311 61.175104-116.734697-1.575781-4.050986-33.792323-7.39594-59.569649 1.253462-8.699541 2.904963 2.766826 16.530355-9.727892 19.440434-46.814006 10.807404-88.175196-12.622623-73.691309-34.645701 14.831785-22.570509 68.571044-9.845564 73.282016-55.124718 2.710549-25.935927 4.956549-55.974003 6.45968-78.298935 63.004648 9.852726 56.069163-81.7677-37.613081-91.574381 189.525561 2.217349 350.128371 124.61565 409.062591 294.48692-2.980683-2.718734-6.448425-4.371258-10.453365-4.774413-28.324157-70.749511-97.074267-19.547874-73.752703 42.855112-124.959457 96.057172-92.973143 163.052434-51.9179 201.416569 21.603552 20.166931 42.200242 50.496628 55.610754 72.279247-14.596442 42.561444 53.781211 25.518448 87.503954-46.709636C890.911457 782.384621 771.249286 898.633281 620.440877 936.783561z" fill="#ffffff" p-id="5606"></path></svg>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onUnmounted } from 'vue';
// import * as Cesium from 'cesium';
import { MeasureTool } from '../utils/measure';
import { MarkerTool, ViewTool } from '../utils/mapTools';


export default defineComponent({
  name: 'ToolBar',
  setup() {
    const measuring = ref(false);
    const marking = ref(false);
    let measureTool: MeasureTool | null = null;
    let markerTool: MarkerTool | null = null;
    let viewTool: ViewTool | null = null;

    // 初始化工具
    const initTools = () => {
      const viewer = (window as any).cesiumViewer;
      if (viewer) {
        if (!markerTool) {
          markerTool = new MarkerTool(viewer, () => {
            marking.value = false;
          });
        }
        if (!viewTool) {
          viewTool = new ViewTool(viewer);
        }
      }
    };

    // 测距工具
    const toggleMeasure = () => {
      measuring.value = !measuring.value;
      const viewer = (window as any).cesiumViewer;
      
      if (measuring.value) {
        measureTool = new MeasureTool(
          viewer,
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

    // 标记工具
    const toggleMarker = () => {
      initTools();
      
      if (!markerTool) return;
      
      marking.value = !marking.value;
      
      if (marking.value) {
        markerTool.start();
      } else {
        markerTool.stop();
      }
    };

    // 重置视图
    const resetView = () => {
      initTools();
      
      if (!viewTool) return;
      
      viewTool.resetView();
    };

    // 组件卸载时清理资源
    onUnmounted(() => {
      if (measureTool) {
        measureTool.clear();
        measureTool = null;
      }
      
      if (markerTool) {
        markerTool.stop();
        markerTool = null;
      }
    });

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
  left: 16px;
  top: 96px; /* 调整顶部距离，避免与菜单栏重叠 */
  background: var(--bg-dark);
  border-radius: var(--radius-md);
  padding: 0px;
  z-index: 1000;
  display: flex;
  flex-direction: column;
  gap: 8px;
  box-shadow: var(--shadow-md);
  border: 1px solid var(--border-color);
  backdrop-filter: blur(12px);
}

.tool-item {
  padding: 12px;
  color: white;
  cursor: pointer;
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  gap: 6px;
  border-radius: var(--radius-sm);
  transition: all var(--transition-normal);
  border: 1px solid transparent;
}


.tool-item:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: var(--border-hover);
  transform: translateY(-2px);
  box-shadow: var(--shadow-sm);
}

.tool-item:hover i {
  color: var(--accent-color);
  transform: scale(1.1);
}

.tool-item.active {
  background: rgba(var(--primary-color-rgb), 0.2);
  border-color: var(--primary-color);
}

.tool-item.active i {
  color: var(--primary-color);
}

@media (max-width: 768px) {
  .toolbar {
    left: 12px;
    top: 80px;
    padding: 8px;
  }
  
  .tool-item {
    padding: 10px;
    min-width: 56px;
  }
  
  .tool-item i {
    font-size: 16px;
  }
  
  .tool-item span {
    font-size: 11px;
  }
}
</style>
