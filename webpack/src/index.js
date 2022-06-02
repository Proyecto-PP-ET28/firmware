import './style/style.scss';
import Chart from 'chart.js/auto';

const chartUpdateInterval = 100;
const maxElementsInChart = 20;
let timeLabel = 0;

const debug = true;

const gateway = `ws://${window.location.hostname}/ws`;
let websocket;
window.addEventListener('load', onload);

function onload(event) {
  initWebSocket();
}

function getValues() {
  websocket.send('getValues');
}

function initWebSocket() {
  if (debug) console.log('Trying to open a WebSocket connection…');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  if (debug) console.log('Connection opened');
  getValues();
}

function onClose(event) {
  if (debug) console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  const currentValues = JSON.parse(event.data);
  document.getElementById('slider1').value = currentValues.motorPWM;
  document.getElementById('slider2').value = currentValues.batteryLevel;
  document.getElementById('slider1-value').innerHTML = currentValues.motorPWM;
  document.getElementById('slider2-value').innerHTML =
    currentValues.batteryLevel;
}

function sendSlider1() {
  const sliderValue = document.getElementById('slider1').value;
  websocket.send('S1' + sliderValue.toString());
  getSlider1Value();
}

function sendSlider2() {
  const sliderValue = document.getElementById('slider2').value;
  websocket.send('S2' + sliderValue.toString());
  getSlider2Value();
}

function getSlider1Value() {
  const xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById('slider1-value').innerHTML = this.responseText;
    }
  };
  xhttp.open('GET', '/S1', true);
  xhttp.send();
}

function getSlider2Value() {
  const xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById('slider2-value').innerHTML = this.responseText;
    }
  };
  xhttp.open('GET', '/S2', true);
  xhttp.send();
}

const thrustConfig = {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        data: [],
        label: 'Thrust',
        fill: false,
        borderColor: '#ff6384',
        tension: 0.1,
      },
    ],
  },
  options: {
    responsive: true,
    scales: {
      y: {
        max: 100,
        min: 0,
      },
    },
  },
};

const rpmConfig = {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        data: [],
        label: 'RPM',
        fill: false,
        borderColor: '#36a2eb',
        tension: 0.1,
      },
    ],
  },
  options: {
    responsive: true,
    scales: {
      y: {
        max: 100,
        min: 0,
      },
    },
  },
};

const ampConfig = {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        data: [],
        label: 'Amp',
        fill: false,
        borderColor: '#ffcd56',
        tension: 0.1,
      },
    ],
  },
  options: {
    responsive: true,
    scales: {
      y: {
        max: 100,
        min: 0,
      },
    },
  },
};

const voltConfig = {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        data: [],
        label: 'Volt',
        fill: false,
        borderColor: '#9966ff',
        tension: 0.1,
      },
    ],
  },
  options: {
    responsive: true,
    scales: {
      y: {
        max: 100,
        min: 0,
      },
    },
  },
};

const ctxThrust = document.getElementById('thrust').getContext('2d');
const thrust = new Chart(ctxThrust, thrustConfig);

const ctxRpm = document.getElementById('rpm').getContext('2d');
const rpm = new Chart(ctxRpm, rpmConfig);

const ctxAmp = document.getElementById('amp').getContext('2d');
const amp = new Chart(ctxAmp, ampConfig);

const ctxVolt = document.getElementById('volt').getContext('2d');
const volt = new Chart(ctxVolt, voltConfig);

function updateData() {
  timeLabel = Math.round((timeLabel + chartUpdateInterval / 1000) * 10) / 10;
  addData(thrust, timeLabel, Math.floor(Math.random() * 100));
  addData(rpm, timeLabel, Math.floor(Math.random() * 100));
  addData(amp, timeLabel, Math.floor(Math.random() * 100));
  addData(volt, timeLabel, Math.floor(Math.random() * 100));
  setTimeout(updateData, chartUpdateInterval);
}

function addData(chart, label, data) {
  chart.data.labels.push(label);
  chart.data.datasets.forEach((dataset) => {
    dataset.data.push(data);
  });
  chart.update();
  if (chart.data.datasets[0].data.length > maxElementsInChart) {
    chart.data.labels.splice(0, 1);
    chart.data.datasets.forEach((dataset) => {
      dataset.data.splice(0, 1);
    });
  }
  chart.update();
}

updateData();