import './style/style.scss';
import { createCharts } from '../src/app/charts/charts';
import BindEvent from './app/listeners';
import WS from './app/websocket';

createCharts();
BindEvent.all();
window.addEventListener('load', WS.init);

  // v.thrust = Math.floor(Math.random() * 100);
  // v.rpm = Math.floor(Math.random() * 100);
  // v.amp = Math.floor(Math.random() * 100);
  // v.volt = Math.floor(Math.random() * 25);