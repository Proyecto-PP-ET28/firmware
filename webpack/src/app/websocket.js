import v from './variables';
import Update from './logic';

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

  static getThrust() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        v.thrust = parseInt(this.responseText, 10);
        if (v.thrust > v.thrustMax) v.thrustMax = v.thrust;
      }
    };
    xhttp.open('GET', '/thrust', true);
    xhttp.send();
  }

  static getRpm() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        v.rpm = parseInt(this.responseText, 10);
        if (v.rpm > v.rpmMax) v.rpmMax = v.rpm;
      }
    };
    xhttp.open('GET', '/rpm', true);
    xhttp.send();
  }

  static getVolt() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        v.volt = parseFloat(this.responseText, 10);
        if (v.volt > v.voltMax) v.voltMax = v.volt;
      }
    };
    xhttp.open('GET', '/volt', true);
    xhttp.send();
  }

  static getAmp() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        v.amp = parseFloat(this.responseText, 10);
        if (v.amp > v.ampMax) v.ampMax = v.amp;
      }
    };
    xhttp.open('GET', '/amp', true);
    xhttp.send();
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
