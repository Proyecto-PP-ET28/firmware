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

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "Arduino.h"
#include "SPIFFS.h"
#include "esp_adc_cal.h"

// * ---------------------------------------------------------------------- * //
// *                               Globales                                 * //
// * ---------------------------------------------------------------------- * //

bool clientIsConnected = 1;
int batteryLevel = 0;
bool isSliderDown = false;
unsigned long millisRandomRPM;

// * ---------------------------------------------------------------------- * //
// *                               Debugging                                * //
// * ---------------------------------------------------------------------- * //

unsigned long millis_time;
unsigned long millis_time_last;
float fps;
int ms;

// * ---------------------------------------------------------------------- * //
// *                                 WiFi                                   * //
// * ---------------------------------------------------------------------- * //

#define WIFI_STATUS 2 // LED en GPIO 2

// Credenciales
const char *LOCAL_SSID = "Fibertel WiFi666 2.4GHz";
const char *LOCAL_PASS = "0044304973";

// * ---------------------------------------------------------------------- * //
// *                              Display (OLED)                            * //
// * ---------------------------------------------------------------------- * //
// #define OLED_SCL 26
// #define OLED_SDA 25
// #define OLED_RES 33
// #define OLED_DC 32
// #define OLED_CS 80  // N/C

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

// * ---------------------------------------------------------------------- * //
// *                               Encoder (EN)                             * //
// * ---------------------------------------------------------------------- * //
 
#define EN_CLK 13
#define EN_DT 12
#define EN_SW 14
#define DEBOUNCE_TIME 30    // Botón

unsigned long debounceMillis = 0;
bool btnState = true;

// * ---------------------------------------------------------------------- * //
// *                          Celda de Carga (LOAD)                         * //
// * ---------------------------------------------------------------------- * //

#define LOAD_SCK 0
#define LOAD_DT 4
#define LOAD_CAL_VALUE -435
#define THRUST_READING_DELAY 200

unsigned long trustLastMillis = 0;
int thrust = 0;
int thrustMax = 0;

// * ---------------------------------------------------------------------- * //
// *                               Sensor IR                                * //
// * ---------------------------------------------------------------------- * //

#define IR_SENSOR 3

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

// * ---------------------------------------------------------------------- * //
// *                                 ESC                                    * //
// * ---------------------------------------------------------------------- * //

#define ESC_PWM 1       
#define ESC_INIT_TIME 1500
#define MIN_PWM_VAL 0
#define MAX_PWM_VAL 180

int motorPWM = 0;

// * ---------------------------------------------------------------------- * //
// *                                 ADCs                                   * //
// * ---------------------------------------------------------------------- * //

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

// * ---------------------------------------------------------------------- * //
// *                                 Menú                                   * //
// * ---------------------------------------------------------------------- * //

#define MENU_SIZE 8
#define menuItem0Y 19
#define menuItem1Y 30
#define menuItem2Y 41
#define menuItem3Y 52
#define menuItemSideMargin 3

String menuItem[MENU_SIZE] = {"< Volver", "Item 2", "Item 3", "Item 4", "Item 5", "Item 6", "Item 7", "Item 8"};
String menuItemValue[MENU_SIZE] = {"", "ON", "OFF", "ON", "1.25", "ON", "1", "4"};
int currentMenuIndex = 0;
int displayMenuIndex = 0;
int displayMenuSelected = 0;
bool isMenuOpen = false;

// * -------------------------------------------------------------------------- * //
// *                                 FUNCIONES                                  * //
// * -------------------------------------------------------------------------- * //

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
void initSD(); 
void saveDataToSD();
void appendFile(fs::FS &fs, const char * path, const char * message);

// * -------------------------------------------------------------------------- * //
// *                                 BITMAPS                                    * //
// * -------------------------------------------------------------------------- * //

#define battery_width 10
#define battery_height 6

const unsigned char battery_bits[] U8X8_PROGMEM = {
    0xfe, 0x00, 0x01, 0x01, 0x01, 0x03, 0x01, 0x03, 0x01, 0x01, 0xfe, 0x00};

#define bottom_bar_width 128
#define bottom_bar_height 10
static unsigned char bottom_bar_bits[] U8X8_PROGMEM = {
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
