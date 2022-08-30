#include "main.hpp"

bool debug = true;              // TODO: Habilita o deshabilita debugging, según corresponda.

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// ! U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, 23, 12, 11, 8, 9); //* reset=*/ U8X8_PIN_NONE);
MD_REncoder Encoder = MD_REncoder(EN_CLK, EN_DT);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar currentValues;
WiFiManager wifiManager;
QRCode qrcode;
HX711 Load;
Servo ESC;

// * -------------------------------------------------------------------------- * //
// *                               INTERRUPCIONES                               * //
// * --------------------------------------------------------------------------- * //

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
  // TODO : Incluir en el menú una opción para medir, y guardar los datos a la SD
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

void IRAM_ATTR Ext_INT3_ISR()  // The interrupt runs this to calculate the period between pulses:
{
  PeriodBetweenPulses = micros() - LastTimeWeMeasured;  // Current "micros" minus the old "micros" when the last pulse happens.
                                                        // This will result with the period (microseconds) between both pulses.
                                                        // The way is made, the overflow of the "micros" is not going to cause any issue.

  LastTimeWeMeasured = micros();  // Stores the current micros so the next time we have a pulse we would have something to compare with.

  if (PulseCounter >= AmountOfReadings)  // If counter for amount of readings reach the set limit:
  {
    PeriodAverage = PeriodSum / AmountOfReadings;  // Calculate the final period dividing the sum of all readings by the
                                                   // amount of readings to get the average.
    PulseCounter = 1;                              // Reset the counter to start over. The reset value is 1 because its the minimum setting allowed (1 reading).
    PeriodSum = PeriodBetweenPulses;               // Reset PeriodSum to start a new averaging operation.

    // Change the amount of readings depending on the period between pulses.
    // To be very responsive, ideally we should read every pulse. The problem is that at higher speeds the period gets
    // too low decreasing the accuracy. To get more accurate readings at higher speeds we should get multiple pulses and
    // average the period, but if we do that at lower speeds then we would have readings too far apart (laggy or sluggish).
    // To have both advantages at different speeds, we will change the amount of readings depending on the period between pulses.
    // Remap period to the amount of readings:
    int RemapedAmountOfReadings = map(PeriodBetweenPulses, 40000, 5000, 1, 10);  // Remap the period range to the reading range.
    // 1st value is what are we going to remap. In this case is the PeriodBetweenPulses.
    // 2nd value is the period value when we are going to have only 1 reading. The higher it is, the lower RPM has to be to reach 1 reading.
    // 3rd value is the period value when we are going to have 10 readings. The higher it is, the lower RPM has to be to reach 10 readings.
    // 4th and 5th values are the amount of readings range.
    RemapedAmountOfReadings = constrain(RemapedAmountOfReadings, 1, 10);  // Constrain the value so it doesn't go below or above the limits.
    AmountOfReadings = RemapedAmountOfReadings;                           // Set amount of readings as the remaped value.
  } else {
    PulseCounter++;                               // Increase the counter for amount of readings by 1.
    PeriodSum = PeriodSum + PeriodBetweenPulses;  // Add the periods so later we can average.
  }

}  // End of Pulse_Even

// * -------------------------------------------------------------------------- * //
// *                                    MAIN                                    * //
// * -------------------------------------------------------------------------- * //

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  ESC.attach(ESC_PWM);
  Encoder.begin();
  oledPrintInitScreen();
  initADC();
  initFS();
  initWiFi();
  initWebSocket();
  initServer();                   // Server: Requests al Server
  initOTA();
  // initSD();                       
  initLoadCell();

  pinMode(WIFI_STATUS, OUTPUT);
  pinMode(EN_CLK, INPUT);
  pinMode(EN_DT, INPUT);
  pinMode(EN_SW, INPUT);
  pinMode(IR_SENSOR, INPUT_PULLUP);

  attachInterrupt(EN_CLK, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_DT, Ext_INT1_ISR, CHANGE);
  attachInterrupt(EN_SW, Ext_INT2_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR), Ext_INT3_ISR, RISING);  // Enable interruption pin 2 when going from LOW to HIGH.
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
  // tachometer();
 
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

// * -------------------------------------------------------------------------- * //
// *                                    INITS                                   * //
// * -------------------------------------------------------------------------- * //

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

void initSD(){
    //Antes que nada, debe chequear si la SD se inicializó bien.

    if(!SD.begin()){    //Si la SD no inicializó correctamente
        Serial.println("No se pudo montar la tarjeta"); //imprime mensaje de error.
        return;
    }
    else{
        uint8_t cardType = SD.cardType();                       //Se define el tipo de SD,
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);      //el tamaño,
        uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);  //el espacio total
        uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);    //y el espacio utilizado.

        Serial.printf("\nINFORMACIÓN DE LA TARJETA: \n");

        Serial.print(" Tipo: ");  //Tipo de tarjeta.
        switch (cardType){
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
        }         //Printea el tipo de SD.

        Serial.printf(" Tamaño: %llu MB\n", cardSize);  //Verifica el tamaño de la SD.
        Serial.printf(" Espacio utilizado: %llu MB de %llu MB\n", usedBytes, totalBytes); //Printea el espacio utilizado del total.
        Serial.printf("\n");
    
        return;
    }
}

// * -------------------------------------------------------------------------- * //
// *                                  WEBSOCKET                                 * //
// * -------------------------------------------------------------------------- * //

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

// * -------------------------------------------------------------------------- * //
// *                                    OLED                                    * //
// * -------------------------------------------------------------------------- * //

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
  u8g2.setCursor(column1 - (u8g2.getStrWidth(String(RPM/2).c_str())), row2 + primaryFontHeight);
  u8g2.print(RPM/2);  // rpm
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
  u8g2.drawXBM(118, 0, battery_width, battery_height, battery_bits);
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
  u8g2.drawXBM(0, 54, bottom_bar_width, bottom_bar_height, bottom_bar_bits);
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
    u8g2.drawXBM(64 - (down_arrow_width / 2), 64 - down_arrow_height, down_arrow_width, down_arrow_height, down_arrow_bits);
    // u8g2.drawXBM(10, 64 - down_arrow_height, down_arrow_width, down_arrow_height, down_arrow_bits);
  }

  if (displayMenuIndex > 0) {
    u8g2.drawXBM(64 - (up_arrow_width / 2), 0, up_arrow_width, up_arrow_height, up_arrow_bits);
    // u8g2.drawXBM(10, 0, up_arrow_width, up_arrow_height, up_arrow_bits);
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


// * -------------------------------------------------------------------------- * //
// *                            FUNCIONES DE LA SD                              * //
// * -------------------------------------------------------------------------- * //

void saveDataToCard(){
  // TODO: Implementar pointers
  String data = "{\n\t\"RPM\":\"" + String(average)+
                 "\n\t\"RPM MAX\":\"" + String(RPMMax)+
                 "\n\t\"Empuje\":\"" + String(thrust)+
                 "\n\t\"Empuje MAX\":\"" + String(thrustMax)+
                 "\n\t\"Voltaje\":\"" + String(extBatVolt)+
                 "\n\t\"Voltaje MAX\":\"" + String(extBatVoltMax)+
                 "\n\t\"Corriente\":\"" + String(extBatAmp)+
                 "\n\t\"Corriente MAX\":\"" + String(extBatAmpMax)+
                  "\"\n}\n";

  appendFile(SD, "/test/test.json", data.c_str());
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){

    Serial.printf("Listando %s:\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("No se pudo abrir el directorio");
        return;
    }
    if(!root.isDirectory()){
        Serial.printf("%s no es un directorio\n", dirname);
        return;
    }

    File file = root.openNextFile();
    const char* fileName = file.name();
    unsigned int fileSizeInMB = file.size() / (1024 * 1024);
    unsigned int fileSizeInKB = file.size() / 1024;
    
    while(file){
        if(file.isDirectory() && file.name() != "System Volume Information"){
            Serial.printf("\t📁 %s\n", fileName);
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } 
        else {
            if(fileSizeInMB < 1) {
                if(fileSizeInKB < 1) Serial.printf("\t🗎 %s %u B\n", fileName, file.size());
                else Serial.printf("\t🗎 %s %u KB\n", fileName, fileSizeInKB);
            }
            else Serial.printf("\t🗎 %s %u MB\n", fileName, fileSizeInMB);
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: \n");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
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
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

// * -------------------------------------------------------------------------- * //
// *                                  SENSORES                                  * //
// * -------------------------------------------------------------------------- * //

void readThrust(){
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

void readADCs(){
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

void tachometer(){
    // The following is going to store the two values that might change in the middle of the cycle.
  // We are going to do math and functions with those values and they can create glitches if they change in the
  // middle of the cycle.
  LastTimeCycleMeasure = LastTimeWeMeasured;  // Store the LastTimeWeMeasured in a variable.
  CurrentMicros = micros();                   // Store the micros() in a variable.

  // CurrentMicros should always be higher than LastTimeWeMeasured, but in rare occasions that's not true.
  // I'm not sure why this happens, but my solution is to compare both and if CurrentMicros is lower than
  // LastTimeCycleMeasure I set it as the CurrentMicros.
  // The need of fixing this is that we later use this information to see if pulses stopped.
  if (CurrentMicros < LastTimeCycleMeasure) {
    LastTimeCycleMeasure = CurrentMicros;
  }

  // Calculate the frequency:
  FrequencyRaw = 10000000000 / PeriodAverage;  // Calculate the frequency using the period between pulses.

  // Detect if pulses stopped or frequency is too low, so we can show 0 Frequency:
  if (PeriodBetweenPulses > ZeroTimeout - ZeroDebouncingExtra || CurrentMicros - LastTimeCycleMeasure > ZeroTimeout - ZeroDebouncingExtra) {  // If the pulses are too far apart that we reached the timeout for zero:
    FrequencyRaw = 0;                                                                                                                         // Set frequency as 0.
    ZeroDebouncingExtra = 2000;                                                                                                               // Change the threshold a little so it doesn't bounce.
  } else {
    ZeroDebouncingExtra = 0;  // Reset the threshold to the normal value so it doesn't bounce.
  }

  FrequencyReal = FrequencyRaw / 10000;  // Get frequency without decimals.
                                         // This is not used to calculate RPM but we remove the decimals just in case
                                         // you want to print it.

  // Calculate the RPM:
  RPM = FrequencyRaw / PulsesPerRevolution * 60;  // Frequency divided by amount of pulses per revolution multiply by
                                                  // 60 seconds to get minutes.
  RPM = RPM / 10000;                              // Remove the decimals.

  // Smoothing RPM:
  total = total - readings[readIndex];  // Advance to the next position in the array.
  readings[readIndex] = RPM;            // Takes the value that we are going to smooth.
  total = total + readings[readIndex];  // Add the reading to the total.
  readIndex = readIndex + 1;            // Advance to the next position in the array.

  if (readIndex >= numReadings)  // If we're at the end of the array:
  {
    readIndex = 0;  // Reset array index.
  }

  // Calculate the average:
  average = total / numReadings;  // The average value it's the smoothed result.

  // Print information on the serial monitor:
  // Comment this section if you have a display and you don't need to monitor the values on the serial monitor.
  // This is because disabling this section would make the loop run faster.
  // Serial.print("Period: ");
  // Serial.print(PeriodBetweenPulses);
  // Serial.print("\tReadings: ");
  // Serial.print(AmountOfReadings);
  // Serial.print("\tFrequency: ");
  // Serial.print(FrequencyReal);
  Serial.print("RPM: ");
  Serial.println(RPM/2);
  Serial.print("  Tachometer: ");
  Serial.println(average);
  // End of tachometre
}