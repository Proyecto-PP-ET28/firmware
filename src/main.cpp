#include "declarations.h"

bool debug = true;  // Habilita debugging

U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, OLED_SCL, OLED_SDA, OLED_CS, OLED_DC, OLED_RES);
MD_REncoder Encoder = MD_REncoder(EN_CLK, EN_DT);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiManager wifiManager;
JSONVar currentValues;
QRCode qrcode;
HX711 Load;

//! -------------------------------------------------------------------------- !//
//!                                 INTERRUPTS                                 !//
//! -------------------------------------------------------------------------- !//

void IRAM_ATTR Ext_INT1_ISR() {
  uint8_t rotation = Encoder.read();
  if (rotation) {
    if (rotation == 16) {  // izquierda
      if (Encoder.speed() > 7) motorPWM -= 3;
      if (Encoder.speed() > 15)
        motorPWM -= 5;
      else
        motorPWM--;
      if (motorPWM < 0) motorPWM = 0;
    } else if (rotation == 32) {  // derecha
      if (Encoder.speed() > 7) motorPWM += 3;
      if (Encoder.speed() > 15)
        motorPWM += 5;
      else
        motorPWM++;
      if (motorPWM > 255) motorPWM = 255;
    }
  }
}

void IRAM_ATTR Ext_INT2_ISR() {
  if (!digitalRead(EN_SW) && btnState && millis() > debounceMillis + DEBOUNCE_TIME) {
    debounceMillis = millis();
    btnState = false;
    clientIsConnected = !clientIsConnected;
  }
  if (digitalRead(EN_SW) && millis() > debounceMillis + DEBOUNCE_TIME) {
    debounceMillis = millis();
    btnState = true;
  }
}

//! -------------------------------------------------------------------------- !//
//!                                    MAIN                                    !//
//! -------------------------------------------------------------------------- !//

void setup() {
  if (debug) Serial.begin(115200);
  u8g2.begin();
  oledPrintInitScreen();

  Encoder.begin();
  initFS();
  initWiFi();
  initWebSocket();
  initServer();
  initOTA();
  initLoadCell();

  pinMode(WIFI_STATUS, OUTPUT);
  ledcSetup(MOTOR_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(MOTOR_PWM_PIN, MOTOR_CH);

  pinMode(EN_CLK, INPUT);
  pinMode(EN_DT, INPUT);
  pinMode(EN_SW, INPUT);
  attachInterrupt(EN_CLK, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_DT, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_SW, Ext_INT2_ISR, CHANGE);
}

void loop() {
  ArduinoOTA.handle();
  ws.cleanupClients();

  if (clientIsConnected) {
    oledPrintMainScreen();
  } else {
    oledPrintIP();
  }

  ledcWrite(MOTOR_CH, motorPWM);

  if (millis() > lastMillis + delayTime) {
    lastMillis = millis();
    getThrust();
  }
}

//! -------------------------------------------------------------------------- !//
//!                                    INITS                                   !//
//! -------------------------------------------------------------------------- !//

void initLoadCell() {
  Load.begin(LOAD_DT, LOAD_SCK);
  Load.set_scale(CAL_VALUE);
  Load.tare();
}

void initFS() {
  if (!SPIFFS.begin()) {
    if (debug) Serial.println("An error has occurred while mounting SPIFFS");
  } else {
    if (debug) Serial.println("SPIFFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  // WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  // wifiManager.resetSettings();
  wifiManager.setClass("invert");
  bool res = wifiManager.autoConnect("ESIMA AP");
  if (debug) {
    if (!res) {
      Serial.println("Failed to connect");
    } else {
      Serial.println("Connection successful");
    }
  }
  if (debug) Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    if (debug) Serial.print('.');
    delay(500);
  }
  // if (!MDNS.begin("esima")) {
  //   if (debug) Serial.println("Error starting mDNS");
  //   return;
  // }
  digitalWrite(WIFI_STATUS, HIGH);
  if (debug) Serial.println(WiFi.localIP());
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {  // Envía el index.html cuando se hace el request
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/S1", HTTP_GET, [](AsyncWebServerRequest *request) {  // Envía el estado del led 4 cuando se hace el request
    request->send_P(200, "text/plain", String(motorPWM).c_str());
  });
  server.on("/S2", HTTP_GET, [](AsyncWebServerRequest *request) {  // Envía el valor de PWM del led 2 cuando se hace el request
    request->send_P(200, "text/plain", String(batteryLevel).c_str());
  });
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
}

void initOTA() {
  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else {
          type = "filesystem";
        }
        if (debug) Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        if (debug) Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        if (debug) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        if (debug) Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          if (debug) Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          if (debug) Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          if (debug) Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          if (debug) Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          if (debug) Serial.println("End Failed");
        }
      });
  ArduinoOTA.begin();
}

//! -------------------------------------------------------------------------- !//
//!                                  WEBSOCKET                                 !//
//! -------------------------------------------------------------------------- !//

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      clientIsConnected = 1;
      if (debug) Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      if (debug) Serial.printf("WebSocket client #%u disconnected\n", client->id());
      clientIsConnected = 0;
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
    if (message.indexOf("S1") >= 0) {  // Se ejecuta cunando se actualiza el slider 1
      motorPWM = message.substring(2).toInt();
    }
    if (message.indexOf("S2") >= 0) {  // Se ejecuta cunando se actualiza el slider 2
      batteryLevel = message.substring(2).toInt();
    }
    if (strcmp((char *)data, "getValues") == 0) {  // Devuelve los valores de las variables actuales cunado se hace el request
      notifyClients(getCurrentValues());
    }
  }
}

void notifyClients(String currentValues) {
  ws.textAll(currentValues);
}

String getCurrentValues() {
  currentValues["motorPWM"] = String(motorPWM);
  currentValues["batteryLevel"] = String(batteryLevel);

  String jsonString = JSON.stringify(currentValues);
  return jsonString;
}

//! -------------------------------------------------------------------------- !//
//!                                    OLED                                    !//
//! -------------------------------------------------------------------------- !//

void oledPrintFPS() {
  millis_time_last = millis_time;
  millis_time = millis();
  fps = millis_time - millis_time_last;
  ms = millis_time - millis_time_last;
  fps = round(1000.0 / fps * 1.0);

  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.setCursor(0, 8 + 21);
  u8g2.print("FPS: ");
  u8g2.print((int)fps);
  u8g2.setCursor(0, 18 + 21);
  u8g2.print("MS: ");
  u8g2.print((int)ms);
}

void oledPrintInitScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_pixzillav1_te);
  const int textOffset = (128 - u8g2.getStrWidth(String("Conectando...").c_str())) / 2;
  u8g2.setCursor(textOffset, 28);
  u8g2.print("Conectando...");
  u8g2.sendBuffer();
}

void oledPrintMainScreen() {
  u8g2.clearBuffer();
  if (debug) oledPrintFPS();
  oledPrintBar(true, true);
  oledPrintBattery(batteryLevel, true);
  u8g2.setCursor(0, 12);
  u8g2.setFont(u8g2_font_BBSesque_tr);
  u8g2.print(thrust);
  u8g2.print("g");
  u8g2.sendBuffer();
}

void oledPrintIP() {
  String localIPStr = (WiFi.localIP()[0] + String(".") +
                       WiFi.localIP()[1] + String(".") +
                       WiFi.localIP()[2] + String(".") +
                       WiFi.localIP()[3]);

  uint8_t qrcodeData[qrcode_getBufferSize(2)];
  qrcode_initText(&qrcode, qrcodeData, 2, 1, localIPStr.c_str());

  u8g2.clearBuffer();
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        u8g2.drawBox((x * 2) + 37, y * 2, 2, 2);
      }
    }
  }
  u8g2.setFont(u8g2_font_pixzillav1_te);
  const int textOffset = (128 - u8g2.getStrWidth(localIPStr.c_str())) / 2;
  u8g2.setCursor(textOffset, 64);
  u8g2.print(WiFi.localIP());
  u8g2.sendBuffer();
}

void oledPrintBattery(int level, bool showPercentage) {
  u8g2.drawXBMP(118, 0, battery_width, battery_height, battery_bits);
  if (level >= 66) {
    u8g2.drawLine(124, 2, 124, 3);  // 100%
    u8g2.drawLine(122, 2, 122, 3);
    u8g2.drawLine(120, 2, 120, 3);
  }
  if (level < 66 && level >= 33) {
    u8g2.drawLine(122, 2, 122, 3);  // 66%
    u8g2.drawLine(120, 2, 120, 3);
  }
  if (level < 33 && level >= 10) {
    u8g2.drawLine(120, 2, 120, 3);  // 33%
  }

  if (showPercentage) {
    u8g2.setFont(u8g2_font_p01type_tr);
    const int textOffset = 128 - ((u8g2.getStrWidth(String(level).c_str())) + 18);
    u8g2.setCursor(textOffset, 4);
    u8g2.print(String(level));
    u8g2.setCursor(111, 5);
    u8g2.print("%");
  }
}

void oledPrintBar(bool showNumber, bool convertToPercentage) {
  u8g2.drawXBMP(0, 54, bottom_bar_width, bottom_bar_height, bottom_bar_bits);
  int barWidth = map(motorPWM, 0, 255, 0, 122);
  if (barWidth < 3) barWidth = 3;
  if (motorPWM != 0) u8g2.drawRBox(3, 57, barWidth, 4, 1);

  if (showNumber) {
    u8g2.setCursor(0, 52);
    u8g2.setFont(u8g2_font_tom_thumb_4x6_t_all);
    u8g2.print("PWM:");
    u8g2.setCursor(17, 52);
    u8g2.setFont(u8g2_font_5x8_tf);
    if (!convertToPercentage) {
      u8g2.print(motorPWM);
    } else {
      int percentage = map(motorPWM, 0, 255, 0, 100);
      u8g2.print(percentage);
      u8g2.setFont(u8g2_font_p01type_tr);
      u8g2.print("%");
    }
  }
}

//! -------------------------------------------------------------------------- !//
//!                                  SENSORES                                  !//
//! -------------------------------------------------------------------------- !//

void getThrust() {
  if (Load.wait_ready_timeout(1000)) {
    thrust = Load.get_units(1);
    if (debug) Serial.print("Weight: ");
    if (debug) Serial.print(thrust, 10);
    if (debug) Serial.println("g");
  } else {
    if (debug) Serial.println("HX711 not found");
  }
}