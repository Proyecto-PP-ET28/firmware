import WS from './websocket';
import Dev from './development';
import UI from './UI';
import v from './variables';
import s from './states';

const measurementUpdateInterval = 100;
export const pwmUpdateInterval = 20;
const batteryUpdateInterval = 1000;
let dev = false;

export default class Update {
  static initAll(isDev) {
    dev = isDev;
    // this.thrust();
    // this.rpm();
    // this.volt();
    // this.amp();
    this.data();
    this.battery();
    !dev && this.pwm();
  }

  // static thrust() {
  //   dev ? Dev.getThrust() : WS.getThrust();
  //   UI.updateMeasurement('thrust');
  //   setTimeout(Update.thrust, measurementUpdateInterval);
  // }

  // static rpm() {
  //   dev ? Dev.getRpm() : WS.getRpm();
  //   UI.updateMeasurement('rpm');
  //   setTimeout(Update.rpm, measurementUpdateInterval);
  // }

  // static volt() {
  //   dev ? Dev.getVolt() : WS.getVolt();
  //   UI.updateMeasurement('volt');
  //   setTimeout(Update.volt, measurementUpdateInterval);
  // }

  // static amp() {
  //   dev ? Dev.getAmp() : WS.getAmp();
  //   UI.updateMeasurement('amp');
  //   setTimeout(Update.amp, measurementUpdateInterval);
  // }

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
