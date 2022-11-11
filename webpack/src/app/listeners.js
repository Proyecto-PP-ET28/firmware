import UI from './UI';
import v from './variables';
import s from './states';
import WS from './websocket';
import { pwmUpdateInterval } from './logic';

export default class BindEvent {
  static all() {
    this.pwm();
    this.buttons();
    this.menu();
    this.keyboard();
    this.settingsForm();
    this.settingsBtns('pwm-min');
    this.settingsBtns('pwm-max');
    this.settingsBtns('current-offset');
  }
  static pwm() {
    const slider = document.getElementById('slider');
    const input = document.getElementById('pwm-input');
    const increase = document.getElementById('increase-pwm');
    const decrease = document.getElementById('decrease-pwm');
    const stop = document.getElementById('stop');

    slider.value = 0;
    input.value = 0;
    UI.drawSliderFill();

    [slider, increase, decrease, stop].forEach((element) => {
      element.addEventListener('mousedown', () => {
        s.arePwmControlsActive = true;
        WS.sendString('S_DOWN');
      });
      element.addEventListener('mouseup', () => {
        setTimeout(() => {
          s.arePwmControlsActive = false;
          WS.sendString('S_UP');
        }, pwmUpdateInterval * 15);
      });
    });

    input.addEventListener('focusin', () => {
      s.arePwmControlsActive = true;
      WS.sendString('S_DOWN');
    });

    input.addEventListener('focusout', () => {
      setTimeout(() => {
        s.arePwmControlsActive = false;
        WS.sendString('S_UP');
      }, pwmUpdateInterval * 15);
    });

    slider.addEventListener('input', (e) => {
      input.value = e.target.value;
      v.pwm = e.target.value;
      UI.drawSliderFill();
      WS.sendPWM(slider);
    });

    input.addEventListener('change', (e) => {
      slider.value = e.target.value;
      v.pwm = e.target.value;
      UI.drawSliderFill();
      WS.sendPWM(slider);
    });

    increase.addEventListener('click', () => {
      let val = parseInt(slider.value, 10);
      val += 5;
      if (val > 100) val = 100;
      slider.value = val;
      input.value = val;
      v.pwm = val;
      UI.drawSliderFill();
      WS.sendPWM(slider);
    });

    decrease.addEventListener('click', () => {
      let val = parseInt(slider.value, 10);
      val -= 5;
      if (val < 0) val = 0;
      slider.value = val;
      input.value = val;
      v.pwm = val;
      UI.drawSliderFill();
      WS.sendPWM(slider);
    });

    stop.addEventListener('mousedown', () => {
      slider.value = 0;
      input.value = 0;
      v.pwm = 0;
      UI.drawSliderFill();
      WS.sendPWM(slider);
    });
  }
  static buttons() {
    const snapshot = document.getElementById('snapshot');
    const resetMax = document.getElementById('reset-max');
    const tare = document.getElementById('tare');

    snapshot.addEventListener('click', () => {
      console.log('SNAP');
      WS.sendString('SNAP');
    });
    tare.addEventListener('click', () => {
      console.log('TARE');
      WS.sendString('TARE');
    });
    resetMax.addEventListener('click', () => {
      console.log('RST');
      v.thrustMax = 0;
      v.rpmMax = 0;
      v.voltMax = 0;
      v.ampMax = 0;
      WS.sendString('RST');
    });
  }
  static menu() {
    document.querySelectorAll('.menu-item').forEach((item) => {
      item.addEventListener('click', (e) => {
        if (e.currentTarget.id === 'settings-menu') {
          WS.getSettings(e.currentTarget);
        } else if (e.currentTarget.id === 'measurements-menu'){
          WS.getSavedData(e.currentTarget);
        } else {
          UI.changeMenu(e.currentTarget);
        }
      });
    });
  }
  static keyboard() {
    const slider = document.getElementById('slider');
    const input = document.getElementById('pwm-input');
    window.addEventListener('keydown', (e) => {
      if (e.key === 's' || e.key === 'Escape' || e.key === ' ') {
        e.preventDefault();
        slider.value = 0;
        input.value = 0;
        v.pwm = 0;
        UI.drawSliderFill();
        WS.sendPWM(slider);
      }
      if (e.key === 'ArrowUp') {
        e.preventDefault();
        const current = document.querySelector('.menu-item.active');
        if (current.id === 'settings-menu') {
          document.getElementById('measurements-menu').click();
        } else if (current.id === 'measurements-menu') {
          document.getElementById('home-menu').click();
        }
      }
      if (e.key === 'ArrowDown') {
        e.preventDefault();
        const current = document.querySelector('.menu-item.active');
        if (current.id === 'home-menu') {
          document.getElementById('measurements-menu').click();
        } else if (current.id === 'measurements-menu') {
          document.getElementById('settings-menu').click();
        }
      }
      if (e.key === 'ArrowLeft') {
        e.preventDefault();
        document.getElementById('decrease-pwm').click();
      }
      if (e.key === 'ArrowRight') {
        e.preventDefault();
        document.getElementById('increase-pwm').click();
      }
      if (e.key === 'c') {
        e.preventDefault();
        document.getElementById('snapshot').click();
      }
      if (e.key === 'g') {
        e.preventDefault();
        document.getElementById('recording').click();
      }
      if (e.key === 'r') {
        e.preventDefault();
        document.getElementById('reset-max').click();
      }
    });
  }

  static settingsForm() {
    document.querySelector('form.settings').addEventListener('submit', (e) => {
      e.preventDefault();
      const bladesNumValue = document.getElementById('blades-num').value;
      const displayRealTimeValue = (+document.getElementById(
        'display-real-time'
      ).checked).toString();
      const displayPeakValue = (+document.getElementById('display-peak')
        .checked).toString();
      const pwmMin = document.getElementById('pwm-min').value;
      const pwmMax = document.getElementById('pwm-max').value;
      const currentOffset = document.getElementById('current-offset').value;
      if (bladesNumValue !== v.lastSettingsArray[0]) {
        WS.sendString(`SAVE_BLADES_NUM_${bladesNumValue}`);
      }
      if (displayRealTimeValue !== v.lastSettingsArray[1]) {
        WS.sendString(`SAVE_DISPLAY_REAL_TIME_${displayRealTimeValue}`);
      }
      if (displayPeakValue !== v.lastSettingsArray[2]) {
        WS.sendString(`SAVE_DISPLAY_PEAK_${displayPeakValue}`);
      }
      if (pwmMin !== v.lastSettingsArray[3]) {
        WS.sendString(`SAVE_PWM_MIN_${pwmMin}`);
      }
      if (pwmMax !== v.lastSettingsArray[4]) {
        WS.sendString(`SAVE_PWM_MAX_${pwmMax}`);
      }
      if (currentOffset !== v.lastSettingsArray[5]) {
        WS.sendString(`SAVE_CURRENT_OFFSET_${currentOffset}`);
      }
    });
  }

  static settingsBtns(target) {
    const input = document.getElementById(target);
    const min = parseFloat(input.getAttribute('min'), 10);
    const max = parseFloat(input.getAttribute('max'), 10);
    const step = parseFloat(input.getAttribute('step'), 10);
    document
      .getElementById(`${target}-decrease`)
      .addEventListener('click', () => {
        let inputValue = parseFloat(input.value, 10);
        inputValue -= step;
        if (inputValue < min) inputValue = min;
        input.value = inputValue.toFixed(target === 'current-offset' ? 3 : 2);
      });
    document
      .getElementById(`${target}-increase`)
      .addEventListener('click', () => {
        let inputValue = parseFloat(input.value, 10);
        inputValue += step;
        if (inputValue > max) inputValue = max;
        input.value = inputValue.toFixed(target === 'current-offset' ? 3 : 2);
      });
  }
}
