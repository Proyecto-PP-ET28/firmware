import WS from './websocket';
import Dev from './development';
import UI from './UI';
import v from './variables';
import s from './states';

const measurementUpdateInterval = 100;
export const pwmUpdateInterval = 20;
const batteryUpdateInterval = 500;
let isDev = false;

export default class Update {
  static initAll(dev) {
    isDev = dev;
    this.thrust();
    this.rpm();
    this.volt();
    this.amp();
    this.battery();
    !isDev && this.pwm();
  }
  static thrust() {
    isDev ? Dev.getThrust() : WS.getThrust();
    UI.updateMeasurement('thrust');
    setTimeout(Update.thrust, measurementUpdateInterval);
  }
  static rpm() {
    isDev ? Dev.getRpm() : WS.getRpm();
    UI.updateMeasurement('rpm');
    setTimeout(Update.rpm, measurementUpdateInterval);
  }
  static volt() {
    isDev ? Dev.getVolt() : WS.getVolt();
    UI.updateMeasurement('volt');
    setTimeout(Update.volt, measurementUpdateInterval);
  }
  static amp() {
    isDev ? Dev.getAmp() : WS.getAmp();
    UI.updateMeasurement('amp');
    setTimeout(Update.amp, measurementUpdateInterval);
  }
  static pwm() {
    if (!s.arePwmControlsActive) {
      document.getElementById('slider').value = v.pwm;
      document.getElementById('pwm-input').value = v.pwm;
      UI.drawSliderFill();
    }
    setTimeout(Update.pwm, pwmUpdateInterval);
  }
  static battery() {
    isDev ? Dev.getBattery() : WS.getBattery();
    UI.updateBattery();
    setTimeout(Update.battery, batteryUpdateInterval);
  }
}
