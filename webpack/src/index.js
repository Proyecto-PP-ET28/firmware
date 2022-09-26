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
