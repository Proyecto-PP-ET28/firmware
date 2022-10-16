import './style/style.scss';
import { createCharts } from '../src/app/charts/charts';
import BindEvent from './app/listeners';
import WS from './app/websocket';
import Update from './app/logic';

const isDev = JSON.parse(DEV_ENV);

createCharts();
BindEvent.all();

if (isDev) {
  Update.initAll(isDev);
} else {
  window.addEventListener('load', WS.init);
}
window.addEventListener('load', () => {
  window
    .matchMedia('(prefers-color-scheme: dark)')
    .addEventListener('change', (e) => {
      const lightThemeIcon = document.getElementById('light-theme-icon');
      const darkThemeIcon = document.getElementById('dark-theme-icon');
      if (e.matches) {
        lightThemeIcon.parentNode.removeChild(lightThemeIcon);
        document.head.append(darkThemeIcon);
      } else {
        darkThemeIcon.parentNode.removeChild(darkThemeIcon);
        document.head.append(lightThemeIcon);
      }
    });
});
