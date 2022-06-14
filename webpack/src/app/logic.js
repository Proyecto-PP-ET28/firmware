import WS from './websocket';
import UI from './UI';
import v from './variables';
import s from './states';

const measurementUpdateInterval = 100;
export const pwmUpdateInterval = 20;
const batteryUpdateInterval = 500;

export default class Update {
  static initAll() {
    this.thrust();
    this.rpm();
    this.volt();
    this.amp();
    this.pwm();
    this.battery();
  }
  static thrust() {
    WS.getThrust();
    UI.updateMeasurement('thrust');
    setTimeout(Update.thrust, measurementUpdateInterval);
  }
  static rpm() {
    WS.getRpm();
    UI.updateMeasurement('rpm');
    setTimeout(Update.rpm, measurementUpdateInterval);
  }
  static volt() {
    WS.getVolt();
    UI.updateMeasurement('volt');
    setTimeout(Update.volt, measurementUpdateInterval);
  }
  static amp() {
    WS.getAmp();
    UI.updateMeasurement('amp');
    setTimeout(Update.amp, measurementUpdateInterval);
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
    WS.getBattery();
    UI.updateBattery();
    setTimeout(Update.battery, batteryUpdateInterval);
  }
}
