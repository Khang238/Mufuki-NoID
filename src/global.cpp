// include all global variables

#include <WiFi.h>
#include <Wire.h>
#include <ctype.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <LittleFS.h>
#include <BleGamepad.h>
#include <ArduinoOTA.h>
//#include <BleMouse.h>
#include <ArduinoJson.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <MPU6050_light.h>
#include <Adafruit_NeoPixel.h>

#include "HIDTypes.h"
#include "sdkconfig.h"
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>
#include <NimBLECharacteristic.h>

#include "esp_task_wdt.h"
#include "hidkeyboard.h"
#include "esptinyusb.h"
#include "keyName.h"
#include "img.h"

const String ver = "v2.0.0";

// Hardware objects
Adafruit_NeoPixel l = Adafruit_NeoPixel(1, 48, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel b = Adafruit_NeoPixel(3,  7, NEO_GRB + NEO_KHZ800);

HIDkeyboard dev;
MPU6050 mpu(Wire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// Analog
int deadZone = 32;
float calMax[3] = {2600, 2700, 2800};
float calMin[3] = {2200, 2300, 2300};
float  hallVal[3] = { 0.00,  0.00,  0.00};
int rawVal[3] = {0, 0, 0};
bool doFilter = false;
int filterType = 0;

// Buttons
bool  nowPress[6] = {false};
bool lastPress[6] = {false};
uint8_t layout[6] = {HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2};

int inputHandler = 0; // 0: Digital emulation, 1: Hysteresis handling, 2: Dynamic actuation
float actuation = 0.3; // For digital emulation
float upperThreshold = 0.6; // For hysteresis handling
float lowerThreshold = 0.4; // For hysteresis handling
float windowSize = 0.3; // For dynamic actuation
float windowFoot[3] = {0.00, 0.00, 0.00}; // For dynamic actuation

unsigned long pressTime[4] = {0};
bool holding[4] = {false};
bool bt4Hold = false;
unsigned long bt4time = 0;
bool needReport = false;

// Screen
unsigned long waitIDLE = 0;
unsigned long screenSaveDuration = 5000;
unsigned long screenOffDuration  = 5000;
bool screenWait = false;
bool screenOff  = false;
uint8_t screenBri = 1;
int logoType = 0;
String screenLogo = "Mufuki";

int maxBri = 16;
uint32_t lastDecTime = 0;
uint8_t mode = 0;
uint8_t ledOutput[3] = { 0,0,0 };

bool underGlow = false;
int glowType = 0; // 0: Tap, 1: Ripple, 2: Smooth, 3: Burn-in, 4: Soild
bool applyEffect[3] = {false, false, false};
bool rgb = false;
uint8_t rgbBri = 255;
uint8_t color[] = {255, 255, 255};
uint8_t rainbowStep = 1;
bool doRainbow = false;
int updateInterval = 32;
uint8_t rgbInterval = 10; // in n x 10ms
unsigned long lastRGBUpdate = 0;
unsigned long lastUpdate = 0;

// Conectivity & Layouts
bool alwaysReport = false;
int layoutType = 0;
uint32_t LOOP_INTERVAL_US = 1000; // 1ms = 1000Hz
uint32_t lastLoopTime = 0;
bool fromMenu = false;
String btName = "Mufuki";