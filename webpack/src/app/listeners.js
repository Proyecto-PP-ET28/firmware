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
    const recording = document.getElementById('recording');
    const resetMax = document.getElementById('reset-max');

    snapshot.addEventListener('click', () => {
      console.log('SNAP');
      WS.sendString('SNAP');
    });
    recording.addEventListener('click', () => {
      console.log('REC');
      WS.sendString('REC');
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
        UI.changeMenu(e.target.closest('.menu-item'));
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
}
