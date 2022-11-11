import v from './variables';

export default class UI {
  static drawSliderFill() {
    const slider = document.getElementById('slider');
    slider.style.setProperty('--value', slider.value);
    slider.style.setProperty('--min', slider.min == '' ? '0' : slider.min);
    slider.style.setProperty('--max', slider.max == '' ? '100' : slider.max);
  }
  static updateMeasurement(id) {
    document.querySelector(`#${id}-value div`).innerText = v[id];
    document.querySelector(`#max-${id} div`).innerText = v[`${id}Max`];
  }

  static updateBattery() {
    document.getElementById('battery-level').innerText = v.battery + '%';
    const batIcon = document.querySelector('#battery-icon .active');
    if (batIcon) batIcon.classList.remove('active');
    if (v.battery >= 66) {
      document.querySelector('.battery-icon-100').classList.add('active');
    }
    if (v.battery < 66 && v.battery >= 33) {
      document.querySelector('.battery-icon-66').classList.add('active');
    }
    if (v.battery < 33 && v.battery >= 10) {
      document.querySelector('.battery-icon-33').classList.add('active');
    }
    if (v.battery < 10) {
      document.querySelector('.battery-icon-0').classList.add('active');
    }
  }

  static changeMenu(target) {
    const menuItem = document.querySelector('.menu-item.active');
    if (menuItem) menuItem.classList.remove('active');
    target.classList.add('active');
    const container = document.querySelector('.main-container.active');
    if (container) container.classList.remove('active');
    const containerId = target.id.substring(0, target.id.indexOf('-'));
    document.getElementById(containerId).classList.add('active');
  }

  static updateSettings(settingsArray) {
    document.getElementById('blades-num').value = settingsArray[0];
    document.getElementById('display-real-time').checked = JSON.parse(settingsArray[1]);
    document.getElementById('display-peak').checked = JSON.parse(settingsArray[2]);
    document.getElementById('pwm-min').value = settingsArray[3];
    document.getElementById('pwm-max').value = settingsArray[4];
    document.getElementById('current-offset').value = parseFloat(settingsArray[5], 10).toFixed(3);
  }
}
