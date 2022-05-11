#include <ArduinoOTA.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
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
#include "WebServer.h"

//* Credenciales WiFi
const char *LOCAL_SSID = "";
const char *LOCAL_PASS = "";

//* Display (OLED)
#define OLED_SCL 18
#define OLED_SDA 23
#define OLED_RES 17
#define OLED_DC 27
#define OLED_CS 80  // N/C

//* Encoder (EN)
#define EN_CLK 36
#define EN_DT 39
#define EN_SW 34

//* Celda de carga (LOAD)
#define LOAD_SCK 15
#define LOAD_DT 4
#define CAL_VALUE -435

//* LEDs
#define WIFI_STATUS 2

//* PWM
#define PWM_FREQ 5000
#define PWM_RES 8
#define MOTOR_PWM_PIN 22
#define MOTOR_CH 0

//* Botón
#define DEBOUNCE_TIME 30
unsigned long debounceMillis = 0;
bool btnState = true;

//* Variables globales
String message = "";
bool clientIsConnected = 0;
int motorPWM = 0;
int batteryLevel = 0;
int thrust;

int delayTime = 200;
unsigned long lastMillis = 0;

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
void getThrust();

//* Debug
unsigned long millis_time;
unsigned long millis_time_last;
float fps;
int ms;

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