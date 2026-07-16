// include all global variables
#pragma once

#include <WiFi.h>
#include <Wire.h>
#include <vector>
#include <ctype.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <MPU6050_light.h>
#include <Adafruit_NeoPixel.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "HIDTypes.h"
#include "sdkconfig.h"
#include "time.h"

// #include <BleMouse.h>
// #include <BleGamepad.h>
// #include <BleKeyboard.h>
// #include <NimBLEUtils.h>
// #include <NimBLEDevice.h>
// #include <NimBLEServer.h>
// #include <NimBLEHIDDevice.h>
// #include <NimBLECharacteristic.h>

#include "esp_task_wdt.h"
#include "hidkeyboard.h"
#include "hidgamepad.h"
#include "esptinyusb.h"
#include "hidmouse.h"
#include "keyName.h"
#include "cdcusb.h"
#include "img.h"

#include "font.h"

extern const String ver;
extern bool firstTime;

extern int  usbMode;
extern bool withBLE;
extern int profileVersion;
extern int vpidSet;
extern volatile bool menuOpen;

extern Adafruit_NeoPixel l;
extern Adafruit_NeoPixel b;
constexpr bool analogLed = false; // change if using analog LED control

extern HIDkeyboard dev;
extern HIDgamepad gdev;
extern HIDmouse mdev;
extern CDCusb CDCUSBSerial;

// extern BleGamepad* gblue;
// extern BleKeyboard* kblue;
// extern BleMouse* mlue;

extern MPU6050 mpu;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

constexpr int adcPins[3] = {1, 2, 3};        // Hall switch
constexpr int ledPins[3] = {7, 6, 5};        // LED output
constexpr int btnPins[4] = {5, 6, 12, 13};   // Buttons

extern float  hallVal[3];
extern int rawVal[3];

extern bool  nowPress[6];
extern bool lastPress[6];
extern float windowFoot[3];

extern unsigned long pressTime[4];
extern bool holding[4];
extern bool bt4Hold;
extern unsigned long bt4time;
extern bool needReport;

// Screen
extern unsigned long waitIDLE;
extern bool screenWait;
extern bool screenOff;
extern bool timeUpdated;
extern struct tm timeinfo;

extern int maxBri;
extern uint32_t lastDecTime;
extern uint8_t mode;
extern uint8_t ledOutput[3];
extern bool morseKey;

// extern bool underGlow;
extern bool applyEffect[3];
extern unsigned long lastRGBUpdate;
extern unsigned long lastUpdate;
extern int updateInterval;

extern bool alwaysReport;
// extern int layoutType;
extern uint32_t LOOP_INTERVAL_US;
extern uint32_t lastLoopTime;
extern bool fromMenu;
extern String btName;

extern int rate;
extern int lastRate;
extern unsigned long lastRateCheckUpdate;

constexpr uint8_t preLayout[5][6] = {
  {HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2}, // osu!
  {HID_KEY_A, HID_KEY_W, HID_KEY_D, HID_KEY_Q, HID_KEY_E, HID_KEY_S},        // WASD
  {HID_KEY_ARROW_LEFT, HID_KEY_SPACE, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_UP, HID_KEY_ENTER, HID_KEY_ARROW_DOWN}, // Arrow
  {HID_KEY_COPY, HID_KEY_PASTE, HID_KEY_ESCAPE, HID_KEY_VOLUME_UP, HID_KEY_MUTE, HID_KEY_VOLUME_DOWN}, // Shortcut
  {HID_KEY_ENTER, HID_KEY_SPACE, HID_KEY_BACKSPACE, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_RIGHT, HID_KEY_ESCAPE}, // BLE Keyboard, useful for me
};

constexpr uint16_t vpidPair[4][2] = {
  {0x054C, 0x05C4}, // PS4 Controller
  {0x045E, 0x028E}, // Xbox 360 Controller
  {0x057E, 0x2009}, // Switch Pro Controller
  {0x046D, 0xC216}  // Logitech F310
};

void globFont();

void screenSaver(const char* title);

void forceReset();