// include all global variables
#pragma once

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
#include "hidgamepad.h"
#include "esptinyusb.h"
#include "keyName.h"
#include "img.h"

extern const String ver;

extern Adafruit_NeoPixel l;
extern Adafruit_NeoPixel b;
constexpr bool analogLed = false; // change if using analog LED control

extern HIDkeyboard dev;
extern MPU6050 mpu;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

constexpr int adcPins[3] = {1, 2, 3};        // Hall switch
constexpr int ledPins[3] = {7, 6, 5};        // LED output
constexpr int btnPins[4] = {5, 6, 12, 13};   // Buttons

extern int deadZone;
extern float calMax[3];
extern float calMin[3];
extern float  hallVal[3];
extern int rawVal[3];
extern bool doFilter;
extern int filterType;

extern bool  nowPress[6];
extern bool lastPress[6];
extern uint8_t layout[6];

extern int inputHandler;
extern float actuation;
extern float upperThreshold;
extern float lowerThreshold;
extern float windowSize;
extern float windowFoot[3];

extern unsigned long pressTime[4];
extern bool holding[4];
extern bool bt4Hold;
extern unsigned long bt4time;
extern bool needReport;

// Screen
extern unsigned long waitIDLE;
extern unsigned long screenSaveDuration;
extern unsigned long screenOffDuration;
extern bool screenWait;
extern bool screenOff;
extern uint8_t screenBri;
extern int logoType;
extern String screenLogo;

extern int maxBri;
extern uint32_t lastDecTime;
extern uint8_t mode;
extern uint8_t ledOutput[3];

extern bool underGlow;
extern int glowType;
extern bool applyEffect[3];
extern bool rgb;
extern uint8_t rgbBri;
extern uint8_t color[3];
extern uint8_t rainbowStep;
extern bool doRainbow;
extern int updateInterval;
extern uint8_t rgbInterval;
extern unsigned long lastRGBUpdate;
extern unsigned long lastUpdate;

extern bool alwaysReport;
extern int layoutType;
extern uint32_t LOOP_INTERVAL_US;
extern uint32_t lastLoopTime;
extern bool fromMenu;
extern String btName;

constexpr uint8_t preLayout[5][6] = {
  {HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2}, // osu!
  {HID_KEY_A, HID_KEY_W, HID_KEY_D, HID_KEY_Q, HID_KEY_E, HID_KEY_S},        // WASD
  {HID_KEY_ARROW_LEFT, HID_KEY_SPACE, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_UP, HID_KEY_ENTER, HID_KEY_ARROW_DOWN}, // Arrow
  {HID_KEY_COPY, HID_KEY_PASTE, HID_KEY_ESCAPE, HID_KEY_VOLUME_UP, HID_KEY_MUTE, HID_KEY_VOLUME_DOWN}, // Shortcut
  {HID_KEY_ENTER, HID_KEY_SPACE, HID_KEY_BACKSPACE, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_RIGHT, HID_KEY_ESCAPE}, // BLE Keyboard, useful for me
};

void screenSaver(const char* title);