#include "declarations.h"

bool debug = true;  // Habilita debugging

U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, OLED_SCL, OLED_SDA, OLED_CS, OLED_DC, OLED_RES);
MD_REncoder Encoder = MD_REncoder(EN_CLK, EN_DT);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar currentValues;
WiFiManager wifiManager;
QRCode qrcode;
HX711 Load;
Servo ESC;

//! -------------------------------------------------------------------------- !//
//!                               INTERRUPCIONES                               !//
//! -------------------------------------------------------------------------- !//

void IRAM_ATTR Ext_INT1_ISR() {  // PWM utilizando Encoder por interrupción
  uint8_t rotation = Encoder.read();
  if (rotation) {
    if (rotation == 16) {  // izquierda
      if (!isMenuOpen) {
        if (!isSliderDown) motorPWM -= 9;
        if (motorPWM < MIN_PWM_VAL) motorPWM = MIN_PWM_VAL;
      } else {
        if (currentMenuIndex > 0) {
          currentMenuIndex--;
          if (currentMenuIndex < displayMenuIndex) {
            displayMenuIndex--;
          } else {
            displayMenuSelected--;
          }
        }
      }

    } else if (rotation == 32) {  // derecha
      if (!isMenuOpen) {
        if (!isSliderDown) motorPWM += 9;
        if (motorPWM > MAX_PWM_VAL) motorPWM = MAX_PWM_VAL;
      } else {
        if (currentMenuIndex < MENU_SIZE - 1) {
          currentMenuIndex++;
          if (currentMenuIndex > displayMenuIndex + 3) {
            displayMenuIndex++;
          } else {
            displayMenuSelected++;
          }
        }
      }
    }
  }
}

void IRAM_ATTR Ext_INT2_ISR() {  // Switch del Encoder
  if (!digitalRead(EN_SW) && btnState && millis() > debounceMillis + DEBOUNCE_TIME) {
    debounceMillis = millis();
    btnState = false;

    switch (currentMenuIndex) {
      case 0:
        isMenuOpen = !isMenuOpen;
        break;
      case 1:
        if (menuItemValue[currentMenuIndex] == "ON") {
          menuItemValue[currentMenuIndex] = "OFF";
        } else {
          menuItemValue[currentMenuIndex] = "ON";
        }
        break;
      case 2:
        if (menuItemValue[currentMenuIndex] == "ON") {
          menuItemValue[currentMenuIndex] = "OFF";
        } else {
          menuItemValue[currentMenuIndex] = "ON";
        }
        break;
      case 3:
        if (menuItemValue[currentMenuIndex] == "ON") {
          menuItemValue[currentMenuIndex] = "OFF";
        } else {
          menuItemValue[currentMenuIndex] = "ON";
        }
        break;
    }
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
  Serial.begin(115200);
  u8g2.begin();
  ESC.attach(ESC_PWM);
  Encoder.begin();
  oledPrintInitScreen();
  initADC();
  initFS();
  initWiFi();  // Conexión WiFi
  initWebSocket();
  initServer();    // Server: Requests al Server
  initOTA();       // Actualizaciones OTA
  initLoadCell();  // Celda de carga
  pinMode(WIFI_STATUS, OUTPUT);

  pinMode(EN_CLK, INPUT);
  pinMode(EN_DT, INPUT);
  pinMode(EN_SW, INPUT);
  pinMode(IR_SENSOR, INPUT_PULLUP);

  attachInterrupt(EN_CLK, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_DT, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_SW, Ext_INT2_ISR, CHANGE);

  RPM = 9000;
}

void loop() {
  ArduinoOTA.handle();
  ws.cleanupClients();
  ESC.write(motorPWM);
  readThrust();
  readADCs();

  if (thrust > thrustMax) thrustMax = thrust;
  if (RPM > RPMMax) RPMMax = RPM;
  if (extBatVolt > extBatVoltMax) extBatVoltMax = extBatVolt;
  if (extBatAmp > extBatAmpMax) extBatAmpMax = extBatAmp;

  if (millis() > millisRandomRPM + 250) {
    millisRandomRPM = millis();
    int randomVar = random(300);
    if (random(2) == 1) {
      RPM += randomVar;
    } else {
      RPM -= randomVar;
    }
  }

  batteryLevel = intBatVolt;
  if (clientIsConnected) {
    if (isMenuOpen) {
      oledPrintMenu();
    } else {
      oledPrintMainScreen();
    }
  } else {
    oledPrintIP();
  }
}

//! -------------------------------------------------------------------------- !//
//!                                    INITS                                   !//
//! -------------------------------------------------------------------------- !//

void initADC() {
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  ADCVoltFactor = (1 / 4095.0) * 3.3 * (1100 / adc_chars.vref);
}

void initLoadCell() {
  Load.begin(LOAD_DT, LOAD_SCK);
  Load.set_scale(LOAD_CAL_VALUE);
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
  WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  // wifiManager.resetSettings();
  // wifiManager.setClass("invert");
  // bool res = wifiManager.autoConnect("ESIMA AP");
  // if (debug) {
  //   if (!res) {
  //     Serial.println("Failed to connect");
  //   } else {
  //     Serial.println("Connection successful");
  //   }
  // }
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
  server.on("/thrust", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(thrust).c_str());
  });
  server.on("/rpm", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(RPM).c_str());
  });
  server.on("/volt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(extBatVolt).c_str());
  });
  server.on("/amp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(extBatAmp).c_str());
  });
  server.on("/pwm", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(map(motorPWM, MIN_PWM_VAL, MAX_PWM_VAL, 0, 100)).c_str());
  });
  server.on("/bat", HTTP_GET, [](AsyncWebServerRequest *request) {
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
    String message = (char *)data;
    if (message.indexOf("PWM") >= 0) {
      motorPWM = map(message.substring(3).toInt(), 0, 100, MIN_PWM_VAL, MAX_PWM_VAL);
    }
    if (message.indexOf("SNAP") >= 0) {
      Serial.println("Captura");
    }
    if (message.indexOf("REC") >= 0) {
      Serial.println("Grabación");
    }
    if (message.indexOf("RST") >= 0) {
      Serial.println("Reiniciar máximos");
    }
    if (message.indexOf("S_DOWN") >= 0) {
      isSliderDown = true;
    }
    if (message.indexOf("S_UP") >= 0) {
      isSliderDown = false;
    }
    if (strcmp((char *)data, "getValues") == 0) {
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
  oledPrintBar(false);
  // if (debug) oledPrintFPS();
  // oledPrintBattery(batteryLevel, true);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(0, 0, 63, 25, 3);
  u8g2.drawRFrame(65, 0, 63, 25, 3);
  u8g2.drawRFrame(0, 27, 63, 25, 3);
  u8g2.drawRFrame(65, 27, 63, 25, 3);

  u8g2.setFont(primaryFont);
  u8g2.setCursor(column1 - (u8g2.getStrWidth((String(thrust) + String(" g")).c_str())), row1 + primaryFontHeight);
  u8g2.print(thrust);  // thrust
  u8g2.print(" g");
  u8g2.setFont(secondaryFont);
  u8g2.setCursor(column1 - (u8g2.getStrWidth((String(thrustMax) + String(" g")).c_str())), row1 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(thrustMax);  // thrust max
  u8g2.print(" g");

  u8g2.setFont(primaryFont);
  u8g2.setCursor(column1 - (u8g2.getStrWidth(String(RPM).c_str())), row2 + primaryFontHeight);
  u8g2.print(RPM);  // rpm
  u8g2.setFont(secondaryFont);
  u8g2.setCursor(column1 - (u8g2.getStrWidth(String(RPMMax).c_str())), row2 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(RPMMax);  // rpm max

  u8g2.setFont(primaryFont);
  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatVolt) + String(" V")).c_str())), row1 + primaryFontHeight);
  u8g2.print(extBatVolt);  // volt
  u8g2.print(" V");
  u8g2.setFont(secondaryFont);
  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatVoltMax) + String(" V")).c_str())), row1 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(extBatVoltMax);  // volt max
  u8g2.print(" V");

  u8g2.setFont(primaryFont);
  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatAmp) + String(" A")).c_str())), row2 + primaryFontHeight);
  u8g2.print(extBatAmp);  // amp
  u8g2.print(" A");
  u8g2.setFont(secondaryFont);
  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatAmpMax) + String(" A")).c_str())), row2 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(extBatAmpMax);  // amp max
  u8g2.print(" A");

  u8g2.setFont(titleFont);
  u8g2.setCursor(titleColumn1, (5 + row1 + titleFontHeight));
  u8g2.print("EMP");
  u8g2.setCursor(titleColumn1, (5 + row2 + titleFontHeight));
  u8g2.print("RPM");
  u8g2.setCursor(titleColumn2, (5 + row1 + titleFontHeight));
  u8g2.print("V");
  u8g2.setCursor(titleColumn2 + 2, (5 + row2 + titleFontHeight));
  u8g2.print("I");
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
  u8g2.setDrawColor(1);
  u8g2.drawXBMP(0, 54, bottom_bar_width, bottom_bar_height, bottom_bar_bits);
  int barWidth = map(motorPWM, MIN_PWM_VAL, MAX_PWM_VAL, 0, 122);
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
      int percentage = map(motorPWM, MIN_PWM_VAL, MAX_PWM_VAL, 0, 100);
      u8g2.print(percentage);
      u8g2.setFont(u8g2_font_p01type_tr);
      u8g2.print("%");
    }
  }
}

void oledPrintMenu() {
  u8g2.clearBuffer();
  oledPrintBattery(intBatVolt, true);
  u8g2.setCursor(0, 4);
  u8g2.print("13:42");

  switch (displayMenuSelected) {
    case 0:
      u8g2.drawFrame(0, menuItem0Y - 10, 128, 13);
      break;
    case 1:
      u8g2.drawFrame(0, menuItem1Y - 10, 128, 13);
      break;
    case 2:
      u8g2.drawFrame(0, menuItem2Y - 10, 128, 13);
      break;
    case 3:
      u8g2.drawFrame(0, menuItem3Y - 10, 128, 13);
      break;
  }

  if (displayMenuIndex + 3 < MENU_SIZE - 1) {
    u8g2.drawXBMP(64 - (down_arrow_width / 2), 64 - down_arrow_height, down_arrow_width, down_arrow_height, down_arrow_bits);
    // u8g2.drawXBMP(10, 64 - down_arrow_height, down_arrow_width, down_arrow_height, down_arrow_bits);
  }

  if (displayMenuIndex > 0) {
    u8g2.drawXBMP(64 - (up_arrow_width / 2), 0, up_arrow_width, up_arrow_height, up_arrow_bits);
    // u8g2.drawXBMP(10, 0, up_arrow_width, up_arrow_height, up_arrow_bits);
  }

  u8g2.setFont(menuFont);
  u8g2.setCursor(menuItemSideMargin, menuItem0Y);
  u8g2.print(menuItem[displayMenuIndex]);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex]).c_str())), menuItem0Y);
  u8g2.print(menuItemValue[displayMenuIndex]);
  u8g2.setCursor(menuItemSideMargin, menuItem1Y);
  u8g2.print(menuItem[displayMenuIndex + 1]);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex + 1]).c_str())), menuItem1Y);
  u8g2.print(menuItemValue[displayMenuIndex + 1]);
  u8g2.setCursor(menuItemSideMargin, menuItem2Y);
  u8g2.print(menuItem[displayMenuIndex + 2]);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex + 2]).c_str())), menuItem2Y);
  u8g2.print(menuItemValue[displayMenuIndex + 2]);
  u8g2.setCursor(menuItemSideMargin, menuItem3Y);
  u8g2.print(menuItem[displayMenuIndex + 3]);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex + 3]).c_str())), menuItem3Y);
  u8g2.print(menuItemValue[displayMenuIndex + 3]);
  u8g2.sendBuffer();
}

//! -------------------------------------------------------------------------- !//
//!                                  SENSORES                                  !//
//! -------------------------------------------------------------------------- !//

void readThrust() {
  if (millis() > trustLastMillis + THRUST_READING_DELAY) {
    trustLastMillis = millis();
    if (Load.wait_ready_timeout(1000)) {
      thrust = Load.get_units(1);
    } else {
      if (debug) Serial.println("HX711 not found");
    }
  }
}

float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

void readADCs() {
  if (millis() > ADCLastMillis + ADC_READING_DELAY) {
    ADCLastMillis = millis();
    ADCReadingCount++;
    rawIntBatVolt += analogRead(INT_BAT);
    rawExtBatVolt += analogRead(EXT_BAT);
    rawExtBatAmp += analogRead(CURRENT_SENSOR);
    if (ADCReadingCount == ADC_N_READINGS) {
      ADCReadingCount = 0;
      // intBatVolt = mapFloat(rawIntBatVolt / ADC_N_READINGS, ADC_MAP_IN_MIN, ADC_MAP_IN_MAX, ADC_MAP_OUT_MIN, ADC_MAP_OUT_MAX) * INT_BAT_VOLT_DIV_FACTOR * ADCVoltFactor;
      intBatVolt = map(rawIntBatVolt / ADC_N_READINGS, 0, 4095, 0, 100);
      extBatVolt = mapFloat(rawExtBatVolt / ADC_N_READINGS, ADC_MAP_IN_MIN, ADC_MAP_IN_MAX, ADC_MAP_OUT_MIN, ADC_MAP_OUT_MAX) * EXT_BAT_VOLT_DIV_FACTOR * ADCVoltFactor;
      extBatAmp = (mapFloat(rawExtBatAmp / ADC_N_READINGS, ADC_MAP_IN_MIN, ADC_MAP_IN_MAX, ADC_MAP_OUT_MIN, ADC_MAP_OUT_MAX) * ADCVoltFactor - CURRENT_QOV + CURRENT_OFFSET) * CURRENT_SENS;
      rawIntBatVolt = 0;
      rawExtBatVolt = 0;
      rawExtBatAmp = 0;
    }
  }
}