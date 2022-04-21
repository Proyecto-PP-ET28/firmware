#include "declarations.h"

bool debug = false; // Habilita debugging por monitor serie

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar currentValues;

//! -------------------------------------------------------------------------- !//
//!                                    MAIN                                    !//
//! -------------------------------------------------------------------------- !//

void setup() {
  if (debug) Serial.begin(115200);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(WIFI_STATUS, OUTPUT);

  ledcSetup(LED3_CH, PWM_FREQ, PWM_RES);
  ledcSetup(LED4_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED3_PIN, LED3_CH);
  ledcAttachPin(LED4_PIN, LED4_CH);

  initFS();
  initWiFi();
  initWebSocket();
  initServer();
}

void loop() {
  ws.cleanupClients();
  if (millis() > lastMillis + delayTime) {
    lastMillis = millis();
    if (debug) Serial.println(getCurrentValues());
  }
}

//! -------------------------------------------------------------------------- !//
//!                                    INITS                                   !//
//! -------------------------------------------------------------------------- !//

void initFS() {
  if (!SPIFFS.begin()) {
    if (debug) Serial.println("An error has occurred while mounting SPIFFS");
  } else {
    if (debug) Serial.println("SPIFFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  if (debug) Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    if (debug) Serial.print('.');
    delay(500);
  }
  digitalWrite(WIFI_STATUS, HIGH);
  if (debug) Serial.println(WiFi.localIP());
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { //Envía el index.html cuando se hace el request
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/B2", HTTP_GET, [](AsyncWebServerRequest *request) { //Envía el estado del led 4 cuando se hace el request
    request->send_P(200, "text/plain", String(ledState2).c_str());
  });
  server.on("/S2", HTTP_GET, [](AsyncWebServerRequest *request) { //Envía el valor de PWM del led 2 cuando se hace el request
    request->send_P(200, "text/plain", String(ledValue4).c_str());
  });
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
}

//! -------------------------------------------------------------------------- !//
//!                                  WEBSOCKET                                 !//
//! -------------------------------------------------------------------------- !//

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      if (debug) Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      if (debug) Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char *)data;
    if (message.indexOf("B1") >= 0) { // Se ejecuta cunando se presiona el botón 1
      ledState1 = !ledState1;
      digitalWrite(LED1_PIN, ledState1);
    }
    if (message.indexOf("B2") >= 0) { // Se ejecuta cunando se presiona el botón 2
      ledState2 = !ledState2;
      digitalWrite(LED2_PIN, ledState2);
    }
    if (message.indexOf("S1") >= 0) { // Se ejecuta cunando se actualiza el slider 1
      ledValue3 = message.substring(2).toInt();
      ledcWrite(LED3_CH, ledValue3);
    }
    if (message.indexOf("S2") >= 0) { // Se ejecuta cunando se actualiza el slider 2
      ledValue4 = message.substring(2).toInt();
      ledcWrite(LED4_CH, ledValue4);
    }
    if (strcmp((char *)data, "getValues") == 0) { //Devuelve los valores de las variables actuales cunado se hace el request
      notifyClients(getCurrentValues());
    }
  }
}

void notifyClients(String currentValues) {
  ws.textAll(currentValues);
}

String getCurrentValues() {
  currentValues["ledState1"] = String(ledState1);
  currentValues["ledState2"] = String(ledState2);
  currentValues["ledValue3"] = String(ledValue3);
  currentValues["ledValue4"] = String(ledValue4);
  
  String jsonString = JSON.stringify(currentValues);
  return jsonString;
}
