import WS from './websocket';
import Dev from './development';
import UI from './UI';
import v from './variables';
import s from './states';

const measurementUpdateInterval = 400;
export const pwmUpdateInterval = 50;
const batteryUpdateInterval = 2000;
let dev = false;

export default class Update {
  static initAll(isDev) {
    dev = isDev;
    this.data();
    this.battery();
    !dev && this.pwm();
  }

  static data() {
    dev ? Dev.getData() : WS.getData();
    UI.updateMeasurement('thrust');
    UI.updateMeasurement('rpm');
    UI.updateMeasurement('volt');
    UI.updateMeasurement('amp');
    setTimeout(Update.data, measurementUpdateInterval);
  }

  static pwm() {
    if (!s.arePwmControlsActive) {
      WS.getPwm();
      document.getElementById('slider').value = v.pwm;
      document.getElementById('pwm-input').value = v.pwm;
      UI.drawSliderFill();
    }
    setTimeout(Update.pwm, pwmUpdateInterval);
  }

  static battery() {
    dev ? Dev.getBattery() : WS.getBattery();
    UI.updateBattery();
    setTimeout(Update.battery, batteryUpdateInterval);
  }
}
