#include "declarations.h"

bool debug = true;  // TODO: Habilitar/Deshabilitar debugging

Preferences config;
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R2, OLED_SCL, OLED_SDA, OLED_CS, OLED_DC, OLED_RES);
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

void IRAM_ATTR Ext_INT1_ISR() {  // Rotación del encoder
  uint8_t rotation = Encoder.read();
  if (rotation) {
    if (rotation == 16) {  // Izquierda
      if (qrIsVisible) return;
      if (!isMenuOpen) {
        if (!isSliderDown) motorPWM -= 9;
        if (motorPWM < MIN_PWM_VAL) motorPWM = MIN_PWM_VAL;
      } else if (isEditingValue) {
        switch (currentMenuIndex) {
          //* Cantidad de palas
          case 5:
            if (bladesNum > 2) {
              bladesNum--;
              menuItemValue[currentMenuIndex] = String(bladesNum);
            }
            break;

            //* PWM min
          case 6:
            if (minPwmIndex > 0) {
              minPwmIndex--;
            }
            break;

            //* PWM max
          case 7:
            if (maxPwmIndex > 0) {
              maxPwmIndex--;
            }
            break;

            //* Factor de desfase
          case 8:
            if (currentOffsetIndex > 0) {
              currentOffsetIndex--;
            }
            break;
        }
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

    } else if (rotation == 32) {  // Derecha
      if (qrIsVisible) return;
      if (!isMenuOpen) {
        if (!isSliderDown) motorPWM += 9;
        if (motorPWM > MAX_PWM_VAL) motorPWM = MAX_PWM_VAL;
      } else if (isEditingValue) {
        switch (currentMenuIndex) {
          //* Cantidad de palas
          case 5:
            if (bladesNum < 6) {
              bladesNum++;
              menuItemValue[currentMenuIndex] = String(bladesNum);
            }
            break;

            //* PWM min
          case 6:
            if (minPwmIndex < 20) {
              minPwmIndex++;
            }
            break;

            //* PWM max
          case 7:
            if (maxPwmIndex < 20) {
              maxPwmIndex++;
            }
            break;

            //* Factor de desfase
          case 8:
            if (currentOffsetIndex < 20) {
              currentOffsetIndex++;
            }
            break;
        }
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

void IRAM_ATTR Ext_INT2_ISR() {  // Botón del encoder
  if (!digitalRead(EN_SW) && btnState && millis() > debounceMillis + DEBOUNCE_TIME) {
    debounceMillis = millis();
    btnState = false;
    if (qrIsVisible) {
      qrIsVisible = false;
      return;
    }
    switch (currentMenuIndex) {
        //* < Volver
      case 0:
        isMenuOpen = !isMenuOpen;
        if (!isMenuOpen) {
          menuItemValue[1] = "";
          menuItemValue[2] = "";
          menuItemValue[3] = "";
        }
        break;

        //* Capturar
      case 1:
        triggerSnap = true;
        menuItemValue[1] = checkGlyph;
        break;

        //* Tare
      case 2:
        triggerTare = true;
        menuItemValue[2] = checkGlyph;
        break;

        //* Reiniciar max
      case 3:
        RPMMax = 0;
        thrustMax = 0;
        extBatVoltMax = 0;
        extBatAmpMax = 0;
        menuItemValue[3] = checkGlyph;
        break;

        //* Mostrar IP / Codigo QR
      case 4:
        qrIsVisible = true;
        break;

        //* Cantidad de palas
      case 5:
        isEditingValue = !isEditingValue;
        if (!isEditingValue) {
          config.putInt("bladesNum", bladesNum);
        }
        break;

        //* PWM min
      case 6:
        isEditingValue = !isEditingValue;
        if (!isEditingValue) {
          config.putInt("minPwmIndex", minPwmIndex);
        }
        break;

        //* PWM max
      case 7:
        isEditingValue = !isEditingValue;
        if (!isEditingValue) {
          config.putInt("maxPwmIndex", maxPwmIndex);
        }
        break;

        //* Factor de desfase
      case 8:
        isEditingValue = !isEditingValue;
        if (!isEditingValue) {
          config.putInt("currentIndex", currentOffsetIndex);
        }
        break;

        //* Mostrar maximos
      case 9:
        if (displayPeek) {
          displayPeek = false;
          menuItemValue[currentMenuIndex] = "OFF";
        } else {
          displayPeek = true;
          menuItemValue[currentMenuIndex] = "ON";
        }
        config.putBool("displayPeek", displayPeek);
        break;

        //* Mostrar tiempo real
      case 10:
        if (displayRealTime) {
          displayRealTime = false;
          menuItemValue[currentMenuIndex] = "OFF";
        } else {
          displayRealTime = true;
          menuItemValue[currentMenuIndex] = "ON";
        }
        config.putBool("displayRealTime", displayRealTime);
        break;
    }
  }
  if (digitalRead(EN_SW) && millis() > debounceMillis + DEBOUNCE_TIME) {
    debounceMillis = millis();
    btnState = true;
  }
}

void IRAM_ATTR Ext_INT3_ISR() {
  motorPWM = 0;
}

//! -------------------------------------------------------------------------- !//
//!                                    MAIN                                    !//
//! -------------------------------------------------------------------------- !//

const int dataIN = IR_SENSOR;

unsigned long prevMillis = 0;
unsigned long duration;
unsigned long refresh;

bool currentState;
bool prevState = LOW;

void setup() {
  pinMode(dataIN, INPUT);

  Serial.begin(115200);
  config.begin("config", false);
  u8g2.begin();
  ESC.attach(ESC_PWM);
  Encoder.begin();
  oledInitScreen();
  // initLoadCellCalibration();  // Calibración de la celda de carga
  initConfig();
  initADC();
  initFS();
  initWiFi();  // Conexión WiFi
  initWebSocket();
  initServer();    // Server: Requests al Server
  initOTA();       // Actualizaciones OTA
  initLoadCell();  // Celda de carga
  initSD();
  pinMode(WIFI_STATUS, OUTPUT);

  pinMode(EN_CLK, INPUT);
  pinMode(EN_DT, INPUT);
  pinMode(EN_SW, INPUT);
  pinMode(STOP_BTN, INPUT_PULLUP);
  pinMode(IR_SENSOR, INPUT_PULLUP);

  attachInterrupt(EN_CLK, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_DT, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_SW, Ext_INT2_ISR, CHANGE);
  attachInterrupt(STOP_BTN, Ext_INT3_ISR, RISING);
  // attachInterrupt(digitalPinToInterrupt(IR_SENSOR), Ext_INT4_ISR, RISING);  // Enable interruption pin 2 when going from LOW to HIGH.
  delay(1000);  // We sometimes take several readings of the period to average. Since we don't have any readings
                // stored we need a high enough value in micros() so if divided is not going to give negative values.
                // The delay allows the micros() to be high enough for the first few cycles.
}

void loop() {
  ArduinoOTA.handle();
  ws.cleanupClients();
  ESC.write(motorPWM);
  readThrust();
  readADCs();

  if (triggerSnap) {
    triggerSnap = false;
    saveDataToCard();
  }

  if (triggerTare) {
    triggerTare = false;
    Load.tare();
  }

  if (minPwmIndex != lastMinPwmIndex) {
    lastMinPwmIndex = minPwmIndex;
    minPwmMs = minPwmVals[minPwmIndex];
    menuItemValue[6] = String(minPwmMs);
  }

  if (maxPwmIndex != lastMaxPwmIndex) {
    lastMaxPwmIndex = maxPwmIndex;
    maxPwmMs = maxPwmVals[maxPwmIndex];
    menuItemValue[7] = String(maxPwmMs);
  }

  if (currentOffsetIndex != lastCurrentOffsetIndex) {
    lastCurrentOffsetIndex = currentOffsetIndex;
    currentOffset = currentOffsetVals[currentOffsetIndex];
    menuItemValue[8] = String(currentOffset, 3);
  }

  if (thrust > thrustMax) thrustMax = thrust;
  if (RPM > RPMMax) RPMMax = RPM;
  if (extBatVolt > extBatVoltMax) extBatVoltMax = extBatVolt;
  if (extBatAmp > extBatAmpMax) extBatAmpMax = extBatAmp;

  currentState = digitalRead(dataIN);
  if (prevState != currentState) {
    if (currentState == HIGH) {
      duration = (micros() - prevMillis);
      RPM = (60000000 / duration);
      prevMillis = micros();
    }
  }
  prevState = currentState;

  if ((millis() - refresh) >= 100) {
    Serial.println(RPM);
  }

  batteryLevel = intBatVolt;
  if (!qrIsVisible) {
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

void initLoadCellCalibration() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB08_te);
  u8g2.setCursor(0, 8);
  u8g2.print("Secuencia de calibración");
  u8g2.setCursor(0, 19);
  u8g2.print("para la celda de carga");
  u8g2.sendBuffer();

  delay(2000);
  Load.begin(LOAD_DT, LOAD_SCK);
  Load.set_scale();

  u8g2.clearBuffer();
  u8g2.setCursor(0, 8);
  u8g2.print("Tare...");
  u8g2.setCursor(0, 22);
  u8g2.print("Quitar cualquier peso");
  u8g2.setCursor(0, 33);
  u8g2.print("que tenga la celda.");
  u8g2.sendBuffer();

  delay(5000);
  Load.tare();

  u8g2.clearBuffer();
  u8g2.setCursor(0, 8);
  u8g2.print("Tare completado!");
  u8g2.setCursor(0, 22);
  u8g2.print("Colocar un peso conocido");
  u8g2.setCursor(0, 33);
  u8g2.print("sobre la celda.");
  u8g2.sendBuffer();

  delay(5000);
  long reading = Load.get_units(10);

  u8g2.clearBuffer();
  u8g2.setCursor(0, 8);
  u8g2.print("Resultado:");
  u8g2.setCursor(0, 19);
  u8g2.print(reading);
  u8g2.setCursor(0, 33);
  u8g2.println("Factor de calibracion =");
  u8g2.setCursor(0, 44);
  u8g2.println("resultado / p. conocido");
  u8g2.sendBuffer();

  while (true)
    ;
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
  // wifiManager.resetSettings();
  wifiManager.setClass("invert");
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConnectTimeout(6);

  String savedSsid = wifiManager.getWiFiSSID();
  String savedPass = wifiManager.getWiFiPass();

  bool ssidIsSaved = wifiManager.getWiFiIsSaved();
  if (ssidIsSaved) {
    oledPrintWifi("connecting", wifiManager.getWiFiSSID());
  } else {
    oledPrintWifi("creating_ap", "");
  }

  bool res = wifiManager.autoConnect("ESIMA AP");

  if (!res) {
    oledPrintWifi("fail", wifiManager.getWiFiSSID());
    oledPrintWifi("creating_ap", "");
  }

  while (WiFi.status() != WL_CONNECTED) {
    wifiManager.process();

    if (wifiManager.getWiFiSSID() != savedSsid || wifiManager.getWiFiPass() != savedPass) {
      oledPrintWifi("connecting", wifiManager.getWiFiSSID());
    }
  }
  oledPrintWifi("success", wifiManager.getWiFiSSID());
  digitalWrite(WIFI_STATUS, HIGH);
  if (debug) Serial.println(WiFi.localIP());
  delay(1000);
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
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(String(thrust) + "," + String(RPM) + "," + String(extBatVolt) + "," + String(extBatAmp)).c_str());
  });
  server.on("/saved", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readFile(SD, "/data.txt").c_str());
  });
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(String(bladesNum) + "," + String(displayRealTime) + "," + String(displayPeek) + "," + String(minPwmMs) + "," + String(maxPwmMs) + "," + String(currentOffset, 3)).c_str());
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

void initConfig() {
  bladesNum = config.getInt("bladesNum", bladesNum);
  menuItemValue[5] = String(bladesNum);

  const int minIndex = config.getInt("minPwmIndex", minPwmIndex);
  minPwmIndex = minIndex;
  lastMinPwmIndex = minIndex;
  minPwmMs = minPwmVals[minIndex];
  menuItemValue[6] = String(minPwmMs);

  const int maxIndex = config.getInt("maxPwmIndex", maxPwmIndex);
  maxPwmIndex = maxIndex;
  lastMaxPwmIndex = maxIndex;
  maxPwmMs = maxPwmVals[maxIndex];
  menuItemValue[7] = String(maxPwmMs);

  const int offsetIndex = config.getInt("currentIndex", currentOffsetIndex);
  currentOffsetIndex = offsetIndex;
  lastCurrentOffsetIndex = offsetIndex;
  currentOffset = currentOffsetVals[offsetIndex];
  menuItemValue[8] = String(currentOffset, 3);

  displayPeek = config.getBool("displayPeek", displayPeek);
  if (displayPeek) menuItemValue[9] = "ON";
  if (!displayPeek) menuItemValue[9] = "OFF";

  displayRealTime = config.getBool("displayRealTime", displayRealTime);
  if (displayRealTime) menuItemValue[10] = "ON";
  if (!displayRealTime) menuItemValue[10] = "OFF";

  Serial.println(bladesNum);
  Serial.println(minPwmMs);
  Serial.println(maxPwmMs);
  Serial.println(currentOffset);
  Serial.println(displayPeek);
  Serial.println(displayRealTime);
}
void initSD() {
  // Antes que nada, debe chequear si la SD se inicializó bien.

  if (!SD.begin()) {                                 // Si la SD no inicializó correctamente
    Serial.println("No se pudo montar la tarjeta");  // imprime mensaje de error.
    return;
  } else {
    uint8_t cardType = SD.cardType();                       // Se define el tipo de SD,
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);      // el tamaño,
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);  // el espacio total
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);    // y el espacio utilizado.

    Serial.printf("\nINFORMACIÓN DE LA TARJETA: \n");

    Serial.print(" Tipo: ");  // Tipo de tarjeta.
    switch (cardType) {
      case CARD_NONE:
        Serial.println("No se insertó una tarjeta SD");
        break;
      case CARD_MMC:
        Serial.println("MMC");
        break;
      case CARD_SD:
        Serial.println("SDSC");
        break;
      case CARD_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("DESCONOCIDO");
        break;
    }  // Printea el tipo de SD.

    Serial.printf(" Tamaño: %llu MB\n", cardSize);                                     // Verifica el tamaño de la SD.
    Serial.printf(" Espacio utilizado: %llu MB de %llu MB\n", usedBytes, totalBytes);  // Printea el espacio utilizado del total.
    Serial.printf("\n");

    return;
  }
}

//! -------------------------------------------------------------------------- !//
//!                                  WEBSOCKET                                 !//
//! -------------------------------------------------------------------------- !//

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      qrIsVisible = false;
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
    String message = (char *)data;
    if (message.indexOf("PWM") >= 0) {
      motorPWM = map(message.substring(3).toInt(), 0, 100, MIN_PWM_VAL, MAX_PWM_VAL);
    }
    if (message.indexOf("SNAP") >= 0) {
      triggerSnap = true;
      Serial.println("Captura");
    }
    if (message.indexOf("TARE") >= 0) {
      triggerTare = true;
    }
    if (message.indexOf("RST") >= 0) {
      RPMMax = 0;
      thrustMax = 0;
      extBatVoltMax = 0;
      extBatAmpMax = 0;
    }
    if (message.indexOf("S_DOWN") >= 0) {
      isSliderDown = true;
    }
    if (message.indexOf("S_UP") >= 0) {
      isSliderDown = false;
    }
    if (message.indexOf("SAVE_BLADES_NUM") >= 0) {
      message.remove(0, String("SAVE_BLADES_NUM").length() + 1);
      bladesNum = message.toInt();
      menuItemValue[5] = message;
      config.putInt("bladesNum", bladesNum);
    }
    if (message.indexOf("SAVE_DISPLAY_REAL_TIME") >= 0) {
      message.remove(0, String("SAVE_DISPLAY_REAL_TIME").length() + 1);
      displayRealTime = message == "1";
      if (displayRealTime)
        menuItemValue[10] = "ON";
      else
        menuItemValue[10] = "OFF";
      config.putBool("displayRealTime", displayRealTime);
    }
    if (message.indexOf("SAVE_DISPLAY_PEAK") >= 0) {
      message.remove(0, String("SAVE_DISPLAY_PEAK").length() + 1);
      displayPeek = message == "1";
      if (displayPeek)
        menuItemValue[9] = "ON";
      else
        menuItemValue[9] = "OFF";
      config.putBool("displayPeek", displayPeek);
    }
    if (message.indexOf("SAVE_PWM_MIN") >= 0) {
      message.remove(0, String("SAVE_PWM_MIN").length() + 1);
      minPwmIndex = round((message.toFloat() - 0.5) / 0.05);
      minPwmMs = minPwmVals[minPwmIndex];
      menuItemValue[6] = String(minPwmMs);
      config.putInt("pwmMinIndex", minPwmIndex);
    }
    if (message.indexOf("SAVE_PWM_MAX") >= 0) {
      message.remove(0, String("SAVE_PWM_MAX").length() + 1);
      maxPwmIndex = round((message.toFloat() - 1.5) / 0.05);
      maxPwmMs = maxPwmVals[maxPwmIndex];
      menuItemValue[7] = String(maxPwmMs);
      config.putInt("pwmMinIndex", maxPwmIndex);
    }
    if (message.indexOf("SAVE_CURRENT_OFFSET") >= 0) {
      message.remove(0, String("SAVE_CURRENT_OFFSET").length() + 1);
      currentOffsetIndex = round((message.toFloat() - 0.05) / 0.005);
      currentOffset = currentOffsetVals[currentOffsetIndex];
      menuItemValue[8] = String(currentOffset, 3);
      config.putInt("currentIndex", currentOffsetIndex);
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

void oledInitScreen() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(48, 18, esima_logo_width, esima_logo_height, esima_logo_bits);  //-14 -> 34
  u8g2.sendBuffer();
  delay(1000);
  for (int i = 0; i <= 48; i += 4) {
    u8g2.clearBuffer();
    u8g2.drawXBMP(-14 + i, 24, esima_text_width, esima_text_height, esima_text_bits);  // 48 -> 0
    u8g2.drawXBMP(48 - i, 18, esima_logo_width, esima_logo_height, esima_logo_bits);   //-14 -> 34
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 48 - i, 64);
    u8g2.setDrawColor(1);
    u8g2.sendBuffer();
  }
  delay(1000);

  // u8g2.clearBuffer();
  // u8g2.drawXBMP(34, 24, esima_text_width, esima_text_height, esima_text_bits);
  // u8g2.drawXBMP(0, 18, esima_logo_width, esima_logo_height, esima_logo_bits);
  // u8g2.sendBuffer();
  // delay(2000);
}

void oledPrintWifi(String type, String ssid) {
  u8g2.clearBuffer();
  if (type == "connecting") {
    u8g2.setFont(u8g2_font_helvB08_te);
    const int textOffset1 = (128 - u8g2.getStrWidth(String("Intentando conexion a...").c_str())) / 2;
    u8g2.setCursor(textOffset1, 24);
    u8g2.print("Intentando conexion a...");

    u8g2.setFont(u8g2_font_helvR08_tf);
    const int textOffset2 = (128 - u8g2.getStrWidth(String(ssid).c_str())) / 2;
    u8g2.setCursor(textOffset2, 42);
    u8g2.print(ssid);
  }

  if (type == "success") {
    u8g2.setFont(u8g2_font_helvB08_te);
    const int textOffset1 = (128 - u8g2.getStrWidth(String("Conectado a").c_str())) / 2;
    u8g2.setCursor(textOffset1, 24);
    u8g2.print("Conectado a");

    u8g2.setFont(u8g2_font_helvR08_tf);
    const int textOffset2 = (128 - u8g2.getStrWidth(String(ssid).c_str())) / 2;
    u8g2.setCursor(textOffset2, 42);
    u8g2.print(ssid);
  }

  if (type == "fail") {
    u8g2.setFont(u8g2_font_helvB08_te);
    const int textOffset1 = (128 - u8g2.getStrWidth(String("Error conectando a").c_str())) / 2;
    u8g2.setCursor(textOffset1, 24);
    u8g2.print("Error conectando a");

    u8g2.setFont(u8g2_font_helvR08_tf);
    const int textOffset2 = (128 - u8g2.getStrWidth(String(ssid).c_str())) / 2;
    u8g2.setCursor(textOffset2, 42);
    u8g2.print(ssid);
  }

  if (type == "creating_ap") {
    u8g2.setFont(u8g2_font_helvB08_te);
    const int textOffset1 = (128 - u8g2.getStrWidth(String("Creando access point").c_str())) / 2;
    u8g2.setCursor(textOffset1, 24);
    u8g2.print("Creando access point");

    u8g2.setFont(u8g2_font_helvR08_tf);
    const int textOffset2 = (128 - u8g2.getStrWidth(String("\"ESIMA AP\"").c_str())) / 2;
    u8g2.setCursor(textOffset2, 42);
    u8g2.print("\"ESIMA AP\"");
  }

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

  u8g2.setCursor(column1 - (u8g2.getStrWidth(String(RPM).c_str())), row2 + primaryFontHeight);
  u8g2.print(RPM);  // rpm

  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatVolt) + String(" V")).c_str())), row1 + primaryFontHeight);
  u8g2.print(extBatVolt);  // volt
  u8g2.print(" V");

  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatAmp) + String(" A")).c_str())), row2 + primaryFontHeight);
  u8g2.print(extBatAmp);  // amp
  u8g2.print(" A");

  u8g2.setFont(secondaryFont);
  u8g2.setCursor(column1 - (u8g2.getStrWidth((String(thrustMax) + String(" g")).c_str())), row1 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(thrustMax);  // thrust max
  u8g2.print(" g");

  u8g2.setCursor(column1 - (u8g2.getStrWidth(String(RPMMax).c_str())), row2 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(RPMMax);  // rpm max

  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatVoltMax) + String(" V")).c_str())), row1 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(extBatVoltMax);  // volt max
  u8g2.print(" V");

  u8g2.setCursor(column2 - (u8g2.getStrWidth((String(extBatAmpMax) + String(" A")).c_str())), row2 + primaryFontHeight + spacing + secondaryFontHeight);
  u8g2.print(extBatAmpMax);  // amp max
  u8g2.print(" A");

  u8g2.setFont(titleFont);
  u8g2.setCursor(titleColumn1, (5 + row1 + titleFontHeight));
  u8g2.print("EMP");
  u8g2.setCursor(titleColumn1, (5 + row2 + titleFontHeight));
  u8g2.print("RPM");
  u8g2.setCursor(titleColumn2, (5 + row1 + titleFontHeight));
  u8g2.print("T");
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
      if (isEditingValue) {
        const int width = 128 - (menuItemSideMargin * 4) - (u8g2.getStrWidth(String(menuItemValue[currentMenuIndex]).c_str()));
        u8g2.drawBox(width, menuItem0Y - 10, width, 13);
      }
      u8g2.drawFrame(0, menuItem0Y - 10, 128, 13);
      break;
    case 1:
      if (isEditingValue) {
        const int width = 128 - (menuItemSideMargin * 4) - (u8g2.getStrWidth(String(menuItemValue[currentMenuIndex]).c_str()));
        u8g2.drawBox(width, menuItem1Y - 10, width, 13);
      }
      u8g2.drawFrame(0, menuItem1Y - 10, 128, 13);
      break;
    case 2:
      if (isEditingValue) {
        const int width = 128 - (menuItemSideMargin * 4) - (u8g2.getStrWidth(String(menuItemValue[currentMenuIndex]).c_str()));
        u8g2.drawBox(width, menuItem2Y - 10, width, 13);
      }
      u8g2.drawFrame(0, menuItem2Y - 10, 128, 13);
      break;
    case 3:
      if (isEditingValue) {
        const int width = 128 - (menuItemSideMargin * 4) - (u8g2.getStrWidth(String(menuItemValue[currentMenuIndex]).c_str()));
        u8g2.drawBox(width, menuItem3Y - 10, width, 13);
      }
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
  if (isEditingValue && displayMenuSelected == 0) u8g2.setDrawColor(0);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex]).c_str())), menuItem0Y);
  u8g2.print(menuItemValue[displayMenuIndex]);
  u8g2.setDrawColor(1);

  u8g2.setCursor(menuItemSideMargin, menuItem1Y);
  u8g2.print(menuItem[displayMenuIndex + 1]);
  if (isEditingValue && displayMenuSelected == 1) u8g2.setDrawColor(0);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex + 1]).c_str())), menuItem1Y);
  u8g2.print(menuItemValue[displayMenuIndex + 1]);
  u8g2.setDrawColor(1);

  u8g2.setCursor(menuItemSideMargin, menuItem2Y);
  u8g2.print(menuItem[displayMenuIndex + 2]);
  if (isEditingValue && displayMenuSelected == 2) u8g2.setDrawColor(0);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex + 2]).c_str())), menuItem2Y);
  u8g2.print(menuItemValue[displayMenuIndex + 2]);
  u8g2.setDrawColor(1);

  u8g2.setCursor(menuItemSideMargin, menuItem3Y);
  u8g2.print(menuItem[displayMenuIndex + 3]);
  if (isEditingValue && displayMenuSelected == 3) u8g2.setDrawColor(0);
  u8g2.setCursor(128 - menuItemSideMargin - (u8g2.getStrWidth(String(menuItemValue[displayMenuIndex + 3]).c_str())), menuItem3Y);
  u8g2.print(menuItemValue[displayMenuIndex + 3]);
  u8g2.setDrawColor(1);
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
      extBatAmp = (mapFloat(rawExtBatAmp / ADC_N_READINGS, ADC_MAP_IN_MIN, ADC_MAP_IN_MAX, ADC_MAP_OUT_MIN, ADC_MAP_OUT_MAX) * ADCVoltFactor - CURRENT_QOV + currentOffset) * CURRENT_SENS;
      rawIntBatVolt = 0;
      rawExtBatVolt = 0;
      rawExtBatAmp = 0;
    }
  }
}

//! -------------------------------------------------------------------------- !//
//!                            FUNCIONES DE LA SD                              !//
//! -------------------------------------------------------------------------- !//

void saveDataToCard() {
  // TODO: Implementar pointers?
  String data = "{\n\t\"RPM\":\"" + String(RPM) + "\"," +
                "\n\t\"RPM MAX\":\"" + String(RPMMax) + "\"," +
                "\n\t\"Empuje\":\"" + String(thrust) + "\"," +
                "\n\t\"Empuje MAX\":\"" + String(thrustMax) + "\"," +
                "\n\t\"Voltaje\":\"" + String(extBatVolt) + "\"," +
                "\n\t\"Voltaje MAX\":\"" + String(extBatVoltMax) + "\"," +
                "\n\t\"Corriente\":\"" + String(extBatAmp) + "\"," +
                "\n\t\"Corriente MAX\":\"" + String(extBatAmpMax) +
                "\"\n}\n###\n";

  appendFile(SD, "/data.txt", data.c_str());
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listando %s:\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("No se pudo abrir el directorio");
    return;
  }
  if (!root.isDirectory()) {
    Serial.printf("%s no es un directorio\n", dirname);
    return;
  }

  File file = root.openNextFile();
  const char *fileName = file.name();
  unsigned int fileSizeInMB = file.size() / (1024 * 1024);
  unsigned int fileSizeInKB = file.size() / 1024;

  while (file) {
    if (file.isDirectory() && file.name() != "System Volume Information") {
      Serial.printf("\t📁 %s\n", fileName);
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      if (fileSizeInMB < 1) {
        if (fileSizeInKB < 1)
          Serial.printf("\t🗎 %s %u B\n", fileName, file.size());
        else
          Serial.printf("\t🗎 %s %u KB\n", fileName, fileSizeInKB);
      } else
        Serial.printf("\t🗎 %s %u MB\n", fileName, fileSizeInMB);
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return String("Error: failed to open file");
  }
  String content = "";
  Serial.print("Read from file: \n");
  while (file.available()) {
    char c = file.read();
    if (isPrintable(c)) {  //
      content.concat(c);
    }
  }
  file.close();
  return content;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}