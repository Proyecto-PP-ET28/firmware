#include <ArduinoOTA.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <HX711.h>
#include <MD_REncoder.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <qrcode.h>

#include "Arduino.h"
#include "SPIFFS.h"
#include "esp_adc_cal.h"

//* Credenciales WiFi
const char *LOCAL_SSID = "";
const char *LOCAL_PASS = "";

//* Display (OLED)
#define OLED_SCL 26
#define OLED_SDA 25
#define OLED_RES 33
#define OLED_DC 32
#define OLED_CS 80  // N/C

#define primaryFont u8g2_font_NokiaSmallBold_tr
#define secondaryFont u8g2_font_smallsimple_tr
#define titleFont u8g2_font_nokiafc22_tu
#define menuFont u8g2_font_NokiaSmallPlain_tf

#define primaryFontHeight 7
#define secondaryFontHeight 6
#define titleFontHeight 7
#define menuFontHeight 7

#define row1 4
#define row2 31
#define column1 59
#define column2 124
#define titleColumn1 3
#define titleColumn2 68
#define spacing 4

//* Encoder (EN)
#define EN_CLK 27
#define EN_DT 14
#define EN_SW 13

//* Celda de carga (LOAD)
#define LOAD_SCK 0
#define LOAD_DT 4
#define LOAD_CAL_VALUE -435
#define THRUST_READING_DELAY 200
unsigned long trustLastMillis = 0;
int thrust = 0;
int thrustMax = 0;

//* Sensor IR
#define IR_SENSOR 16
int RPM = 0;
int RPMMax = 0;

//* LED
#define WIFI_STATUS 2

//* ESC
#define ESC_PWM 22
#define ESC_INIT_TIME 1500
#define MIN_PWM_VAL 0
#define MAX_PWM_VAL 180
int motorPWM = 0;

//* ADCs
#define INT_BAT 36
#define EXT_BAT 39
#define CURRENT_SENSOR 34
#define EXT_BAT_VOLT_DIV_FACTOR 11
#define INT_BAT_VOLT_DIV_FACTOR 1
#define ADC_N_READINGS 3
#define ADC_READING_DELAY 1
#define ADC_MAP_IN_MIN 0.02
#define ADC_MAP_IN_MAX 2.60
#define ADC_MAP_OUT_MIN 0.12
#define ADC_MAP_OUT_MAX 2.75
#define CURRENT_SENS 25
#define CURRENT_OFFSET 0.095
#define CURRENT_QOV 2.5
unsigned long ADCLastMillis = 0;
int ADCReadingCount = 0;
float ADCVoltFactor = 0;
float rawIntBatVolt = 0;
float rawExtBatVolt = 0;
float rawExtBatAmp = 0;
float intBatVolt = 0;
float extBatVolt = 0;
float extBatAmp = 0;

float extBatVoltMax = 0;
float extBatAmpMax = 0;

//* Botón
#define DEBOUNCE_TIME 30
unsigned long debounceMillis = 0;
bool btnState = true;

//* Menú
#define MENU_SIZE 8
String menuItem[MENU_SIZE] = {"< Volver", "Item 2", "Item 3", "Item 4", "Item 5", "Item 6", "Item 7", "Item 8"};
String menuItemValue[MENU_SIZE] = {"", "ON", "OFF", "ON", "1.25", "ON", "1", "4"};
int currentMenuIndex = 0;
int displayMenuIndex = 0;
int displayMenuSelected = 0;

#define menuItem0Y 19
#define menuItem1Y 30
#define menuItem2Y 41
#define menuItem3Y 52
#define menuItemSideMargin 3

bool isMenuOpen = false;

//* Variables globales
bool clientIsConnected = 1;
int batteryLevel = 0;
bool isSliderDown = false;
unsigned long millisRandomRPM;

//* Debug
unsigned long millis_time;
unsigned long millis_time_last;
float fps;
int ms;

//! -------------------------------------------------------------------------- !//
//!                                  FUNCIONES                                 !//
//! -------------------------------------------------------------------------- !//

String getCurrentValues();
void initFS();
void initWiFi();
void initWebSocket();
void initServer();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void notifyClients(String sliderValues);
void oledPrintIP();
void oledPrintFPS();
void oledPrintBattery(int level, bool percentage);
void oledPrintBar(bool showNumber, bool convertToPercentage = false);
void Task1code(void *pvParameters);
void oledPrintInitScreen();
void initOTA();
void initLoadCell();
void oledPrintMainScreen();
void readThrust();
void oledPrintMenu();
void initADC();
void readADCs();
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax);

//! -------------------------------------------------------------------------- !//
//!                                   BITMAPS                                  !//
//! -------------------------------------------------------------------------- !//

#define battery_width 10
#define battery_height 6
static const unsigned char battery_bits[] U8X8_PROGMEM = {
    0xfe, 0x00, 0x01, 0x01, 0x01, 0x03, 0x01, 0x03, 0x01, 0x01, 0xfe, 0x00};

#define bottom_bar_width 128
#define bottom_bar_height 10
static const unsigned char bottom_bar_bits[] U8X8_PROGMEM = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0,
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff};

#define down_arrow_width 8
#define down_arrow_height 5
static const unsigned char down_arrow_bits[] U8X8_PROGMEM = {
    0x81, 0xc3, 0x66, 0x3c, 0x18};

#define up_arrow_width 8
#define up_arrow_height 5
static const unsigned char up_arrow_bits[] U8X8_PROGMEM = {
    0x18, 0x3c, 0x66, 0xc3, 0x81};
