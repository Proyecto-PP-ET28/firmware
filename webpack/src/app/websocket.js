import v from './variables';
import Update from './logic';
import UI from './UI';

const gateway = `ws://${window.location.hostname}/ws`;
let websocket;

export default class WS {
  static init() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = WS.onOpen;
    websocket.onclose = WS.onClose;
    websocket.onmessage = WS.onMessage;
  }

  static getValues() {
    websocket.send('getValues');
  }

  static onOpen() {
    console.log('Connection opened');
    WS.getValues();
    Update.initAll();
  }

  static onClose() {
    console.log('Connection closed');
    setTimeout(WS.init, 2000);
  }

  static onMessage(event) {
    console.log(event);
    const currentValues = JSON.parse(event.data);
  }

  static sendPWM(slider) {
    websocket.send('PWM' + slider.value.toString());
  }
  static sendString(string) {
    websocket.send(string);
  }

  static getPwm() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        v.pwm = parseInt(this.responseText, 10);
      }
    };
    xhttp.open('GET', '/pwm', true);
    xhttp.send();
  }

  static getData() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        console.log(' this.responseText: ',  this.responseText);
        const data = this.responseText.split(',');
        v.thrust = parseInt(data[0], 10);
        v.rpm = parseInt(data[1], 10);
        v.volt = parseFloat(data[2], 10).toFixed(2);
        v.amp = parseFloat(data[3], 10).toFixed(2);
        if (v.thrust > v.thrustMax) v.thrustMax = v.thrust;
        if (v.rpm > v.rpmMax) v.rpmMax = v.rpm;
        if (v.volt > v.voltMax) v.voltMax = v.volt;
        if (v.amp > v.ampMax) v.ampMax = v.amp;
      }
    };
    xhttp.open('GET', '/data', true);
    xhttp.send();
  }

  static getSettings(target) {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        console.log(' this.responseText: ',  this.responseText);
        const settingsArray = this.responseText.split(',');
        v.lastSettingsArray = settingsArray;
        console.log('settingsArray: ', settingsArray);
        UI.updateSettings(settingsArray);
        UI.changeMenu(target);
      }
    };
    xhttp.open('GET', '/settings', true);
    xhttp.send();
  }

  static getBattery() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        v.battery = parseInt(this.responseText, 10);
      }
    };
    xhttp.open('GET', '/bat', true);
    xhttp.send();
  }
}
