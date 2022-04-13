#include <Arduino.h>
#include <HX711.h>        // Celda de carga
#include <MD_REncoder.h>  // Encoder
#include <U8g2lib.h>      // Display

#include "declarations.h"
#include "soc/rtc.h"

U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, OLED_SCL, OLED_SDA, OLED_CS, OLED_DC, OLED_RES);
MD_REncoder Encoder = MD_REncoder(EN_CLK, EN_DT);
HX711 Load;

int testVal = 0;
int delayTime = 100;
unsigned long lastMillis = 0;

void setup() {
  //! Esto limita la frecuencia del CPU porque es demasiado rápido para medir los valores de la celda de carga
  // TODO Hay que buscar otra librería u otra solución porque esto no es ideal
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);

  Serial.begin(57600);

  u8g2.begin();
  Encoder.begin();
  Load.begin(LOAD_SCK, LOAD_DT);

  Load.set_scale(CAL_VALUE);  // Asigna el valor de calibración
  Load.tare();                // Asigna el valor del tare
  // Load.get_units(N_READINGS);  // Devuelve el valor del ADC convertido a gramos menos el valor del tare
  printCenterTest(testVal);
}

void loop() {
  uint8_t rotation = Encoder.read();
  if (rotation) {
    if (rotation == 16) {  // left
      testVal--;
      if (testVal < 0) testVal = 0;
      printCenterTest(testVal);
    } else if (rotation == 32) {  // right
      testVal++;
      if (testVal > 10) testVal = 10;
      printCenterTest(testVal);
    }
  }
  // Encoder.speed();
  if (millis() > lastMillis + delayTime) {
    lastMillis = millis();
  }
}

void printCenterTest(int targetVal) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB14_tf);
  String tempString = String(targetVal) + "%";
  int textWidth = u8g2.getStrWidth(tempString.c_str());
  int textOffset = (128 - textWidth) / 2;
  u8g2.setCursor(textOffset, 24);
  u8g2.print(targetVal * 10);
  u8g2.print("%");
  u8g2.drawRFrame(2, 50, 124, 13, 6);
  if (targetVal == 0) {
    u8g2.drawRBox(4, 52, 0, 9, 0);
  }
  else {
    u8g2.drawRBox(4, 52, targetVal * 12, 9, 4);
  }
  u8g2.sendBuffer();
}