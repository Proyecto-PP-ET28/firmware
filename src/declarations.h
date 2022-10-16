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
#include <Preferences.h>

#include "Arduino.h"
#include "SPIFFS.h"
#include "esp_adc_cal.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

//* Credenciales WiFi
const char *LOCAL_SSID = "Fibertel WiFi617 2.4GHz";
const char *LOCAL_PASS = "0041781818";
// const char *LOCAL_SSID = "300 de paleta";
// const char *LOCAL_PASS = "berenjenas";

//* Display (OLED)
#define OLED_SCL 26
#define OLED_SDA 25
#define OLED_RES 33
#define OLED_DC 32
#define OLED_CS 80  // No conectado

#define primaryFont u8g2_font_NokiaSmallBold_tr
#define secondaryFont u8g2_font_smallsimple_tr
#define titleFont u8g2_font_nokiafc22_tu
#define menuFont u8g2_font_NokiaSmallPlain_tf

String checkGlyph = "#";

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
const byte PulsesPerRevolution = 2;                               // Establece cuántos pulsos hay en cada revolución. Default: 2.
const unsigned long ZeroTimeout = 100000;                         // Para un mayor tiempo de respuesta, un buen valor sería 100000.
const byte numReadings = 2;                                       // Number of samples for smoothing. The higher, the more smoothing, but it's going to
volatile unsigned long LastTimeWeMeasured;                        // Stores the last time we measured a pulse so we can calculate the period.
volatile unsigned long PeriodBetweenPulses = ZeroTimeout + 1000;  // Stores the period between pulses in microseconds.
                                                                  // It has a big number so it doesn't start with 0 which would be interpreted as a high frequency.
volatile unsigned long PeriodAverage = ZeroTimeout + 1000;        // Stores the period between pulses in microseconds in total, if we are taking multiple pulses.
                                                                  // It has a big number so it doesn't start with 0 which would be interpreted as a high frequency.
unsigned long FrequencyRaw;                                       // Calculated frequency, based on the period. This has a lot of extra decimals without the decimal point.
unsigned long FrequencyReal;                                      // frecuencia sin decimales.
unsigned long RPM;
int RPMMax = 0;                                                   // RPM en crudo, sin ningún procesamiento.
unsigned int PulseCounter = 1;                                    // Counts the amount of pulse readings we took so we can average multiple pulses before calculating the period.

unsigned long PeriodSum;  // Stores the summation of all the periods to do the average.

unsigned long LastTimeCycleMeasure = LastTimeWeMeasured;  // Stores the last time we measure a pulse in that cycle.
unsigned long CurrentMicros = micros();                   // Stores the micros in that cycle.
unsigned int AmountOfReadings = 1;

unsigned int ZeroDebouncingExtra;  // Stores the extra value added to the ZeroTimeout to debounce it.
                                   // The ZeroTimeout needs debouncing so when the value is close to the threshold it
                                   // doesn't jump from 0 to the value. This extra value changes the threshold a little
                                   // when we show a 0.

// Variables for smoothing tachometer:
unsigned long readings[numReadings];  // The input.
unsigned long readIndex;              // The index of the current reading.
unsigned long total;                  // The running total.
unsigned long average;                // The RPM value after applying the smoothing.


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
#define MENU_SIZE 11
String menuItem[MENU_SIZE] = {"< Volver", "Capturar", "Tare", "Reiniciar max", "Mostrar IP / Codigo QR", "Cantidad de palas", "PWM min", "PWM max", "Factor de desfase", "Mostrar maximos", "Mostrar tiempo real"};
String menuItemValue[MENU_SIZE] = {"", "", "", "", "", "2", "1.00", "2.00", "0.100", "ON", "ON"};
int currentMenuIndex = 0;
int displayMenuIndex = 0;
int displayMenuSelected = 0;

#define menuItem0Y 19
#define menuItem1Y 30
#define menuItem2Y 41
#define menuItem3Y 52
#define menuItemSideMargin 3

bool isMenuOpen = false;
bool isEditingValue = false;

//* Config

int bladesNum = 2;
bool displayPeek = true;
bool displayRealTime = true;

float minPwmMs = 1.00;
float maxPwmMs = 2.00;
float currentOffset = 0.095;

int minPwmIndex = 10;
int lastMinPwmIndex = 10;
int maxPwmIndex = 10;
int lastMaxPwmIndex = 10;
int currentOffsetIndex = 10;
int lastCurrentOffsetIndex = 10;

const float minPwmVals[21] = {0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90, 0.95, 1.00, 1.05, 1.10, 1.15, 1.20, 1.25, 1.30, 1.35, 1.40, 1.45, 1.50};
const float maxPwmVals[21] = {1.50, 1.55, 1.60, 1.65, 1.70, 1.75, 1.80, 1.85, 1.90, 1.95, 2.00, 2.05, 2.10, 2.15, 2.20, 2.25, 2.30, 2.35, 2.40, 2.45, 2.50};
const float currentOffsetVals[21] = {0.050, 0.055, 0.060, 0.065, 0.070, 0.075, 0.080, 0.085, 0.090, 0.095, 0.100, 0.105, 0.110, 0.115, 0.120, 0.125, 0.130, 0.135, 0.140, 0.145, 0.150};

//* Variables globales
bool qrIsVisible = true;
int batteryLevel = 0;
bool isSliderDown = false;
unsigned long millisRandomRPM;

bool triggerTare = false;

//* Debug
unsigned long millis_time;
unsigned long millis_time_last;
float fps;
int ms;

//* Tarjeta SD
// TODO: Implementar pointers?
// unsigned long * average_buffer = &average;       //RPM

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
void initConfig();
void readADCs();
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax);
void tachometer();
void initSD(); 
void saveDataToSD();
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);


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
