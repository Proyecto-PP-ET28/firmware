#include "Arduino.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Arduino_JSON.h>
#include "SPIFFS.h"

//* Credenciales WiFi
const char* LOCAL_SSID = "";
const char* LOCAL_PASS = "";

//* LEDs
#define LED1_PIN 19
#define LED2_PIN 18

#define WIFI_STATUS 2

//* PWM
#define PWM_FREQ 5000
#define PWM_RES 8

#define LED3_PIN 5
#define LED3_CH 0
#define LED4_PIN 17
#define LED4_CH 1

//* Variables globales
bool ledState1 = 0;
bool ledState2 = 0;
int ledValue3 = 0;
int ledValue4 = 0;

String message = "";
int delayTime = 1000;
unsigned long lastMillis = 0;

//* Funciones
void initFS();
void initWiFi();
void initWebSocket();
void initServer();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void notifyClients(String sliderValues);
String getCurrentValues();

