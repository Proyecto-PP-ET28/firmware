import v from './variables';

const batLevel = Math.round(Math.random() * 100)

export default class Dev {

  static getData() {
    v.thrust = (1400 + Math.round(Math.random() * 20));
    v.rpm = (8000 + Math.round(Math.random() * 100));
    v.volt = (14 + Math.random() * 4).toFixed(2);
    v.amp = (14 + Math.random() * 4).toFixed(2);
    
    if (v.thrust > v.thrustMax) v.thrustMax = v.thrust;
    if (v.rpm > v.rpmMax) v.rpmMax = v.rpm;
    if (v.volt > v.voltMax) v.voltMax = v.volt;
    if (v.amp > v.ampMax) v.ampMax = v.amp;
  }

  static getBattery() {
    v.battery = batLevel;
  }
}