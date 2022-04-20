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
  document.getElementById("slider1").value = currentValues.ledValue3;
  document.getElementById("slider2").value = currentValues.ledValue4;
  document.getElementById("button2-value").innerHTML = currentValues.ledState2;
  document.getElementById("slider2-value").innerHTML = currentValues.ledValue4;
}

function sendButton1() {
  websocket.send('B1'.toString());
  getValues();
}

function sendButton2() {
  websocket.send('B2'.toString());
  getButton2Value();
  getValues();
}

function sendSlider1() {
  const sliderValue = document.getElementById("slider1").value;
  websocket.send('S1' + sliderValue.toString());
}

function sendSlider2() {
  const sliderValue = document.getElementById("slider2").value;
  websocket.send('S2' + sliderValue.toString());
  getSlider2Value();
}

function getButton2Value() {
  const xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("button2-value").innerHTML = this.responseText;
    }
  };
  xhttp.open('GET', '/B2', true);
  xhttp.send();
}

function getSlider2Value() {
  const xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("slider2-value").innerHTML = this.responseText;
    }
  };
  xhttp.open('GET', '/S2', true);
  xhttp.send();
}

//! Poner las funciones inline en el html o hacer una función para cada input no es lo mejor

// setInterval(function ( ) {
// }, 300 ) ;
