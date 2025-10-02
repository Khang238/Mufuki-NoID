#include <WiFi.h>
#include <Wire.h>
#include <ctype.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <BleMouse.h>
#include <BleKeyboard.h>
#include <BleGamepad.h>
#include <ArduinoOTA.h>
#include <MPU6050_light.h>
#include <Adafruit_NeoPixel.h>

#include "esptinyusb.h"
#include "hidkeyboard.h"
#include "keyName.h"
#include "img.h"


// External
Adafruit_NeoPixel l = Adafruit_NeoPixel(1, 48, NEO_GRB + NEO_KHZ800);
HIDkeyboard dev;
MPU6050 mpu(Wire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

const int adcPins[3] = {1, 2, 3};        // Hall switch
const int ledPins[3] = {7, 6, 5};        // LED output
const int btnPins[4] = {11, 10, 12, 13}; // Buttons

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

// Effects
const int MAX_WAVES      = 5;
const int WAVE_DURATION  = 300;
const int WAVE_DELAY     = 80;
const int DELIGHT        = 50;

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

uint8_t preLayout[][6] = {
  {HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2}, // osu!
  {HID_KEY_A, HID_KEY_W, HID_KEY_D, HID_KEY_Q, HID_KEY_E, HID_KEY_S},        // WASD
  {HID_KEY_ARROW_LEFT, HID_KEY_SPACE, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_UP, HID_KEY_ENTER, HID_KEY_ARROW_DOWN}, // Arrow
  {HID_KEY_COPY, HID_KEY_PASTE, HID_KEY_ESCAPE, HID_KEY_VOLUME_UP, HID_KEY_MUTE, HID_KEY_VOLUME_DOWN}, // Shortcut
  {HID_KEY_ENTER, HID_KEY_SPACE, HID_KEY_BACKSPACE, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_RIGHT, HID_KEY_ESCAPE}, // BLE Keyboard, useful for me
};

// why tf there is so much variables for such a osu keyboard i'm crying (>_<)

struct LedFade {
  bool active;
  uint32_t startTime;
};
LedFade singleFade[3];

void updateSingleFade() {
  uint32_t now = millis();
  for (int i = 0; i < 3; ++i) {
    if (!singleFade[i].active) continue;
    uint32_t elapsed = now - singleFade[i].startTime;
    if (elapsed >= (uint32_t)WAVE_DURATION) {
      singleFade[i].active = false;
      continue;
    }
    uint8_t val;
    if (elapsed < (uint32_t)(WAVE_DURATION/2)) {
      val = (uint8_t)(   ( (uint32_t)elapsed * maxBri )
                       / (uint32_t)(WAVE_DURATION/2) );
    } else {
      uint32_t e2 = elapsed - (WAVE_DURATION/2);
      val = (uint8_t)(   maxBri
                       - ( (uint32_t)e2 * maxBri )
                        / (uint32_t)(WAVE_DURATION/2) );
    }
    if (val > ledOutput[i]) ledOutput[i] = val;
  }
}

struct LedWave {
  bool active;
  uint32_t startTime;
};

struct Wave {
  bool active;
  int center;
  uint32_t createdAt;
  LedWave leds[3];
};

Wave waves[MAX_WAVES];
int waveIdx = 0;

void addRippleWave(int center) {
  Wave &w = waves[waveIdx];
  w.active = true;
  w.center = center;
  w.createdAt = millis();
  for (int i = 0; i < 3; ++i) {
    int dist = abs(center - i);
    w.leds[i].active = true;
    w.leds[i].startTime = w.createdAt + dist * WAVE_DELAY;
  }
  waveIdx = (waveIdx + 1) % MAX_WAVES;
}

void updateRipple() {
  uint32_t now = millis();
  uint8_t brTmp[3] = { 0, 0, 0 };

  for (int w = 0; w < MAX_WAVES; ++w) {
    if (!waves[w].active) continue;
    bool anyOn = false;

    for (int i = 0; i < 3; ++i) {
      if (!waves[w].leds[i].active) continue;
      uint32_t start = waves[w].leds[i].startTime;
      if (now < start) continue;

      uint32_t elapsed = now - start;
      if (elapsed >= (uint32_t)WAVE_DURATION) {
        waves[w].leds[i].active = false;
        continue;
      }
      uint8_t val;
      if (elapsed < (uint32_t)(WAVE_DURATION/2)) {
        val = (uint8_t)(   ( (uint32_t)elapsed * maxBri )
                         / (uint32_t)(WAVE_DURATION/2) );
      } else {
        uint32_t e2 = elapsed - (WAVE_DURATION/2);
        val = (uint8_t)(   maxBri
                         - ( (uint32_t)e2 * maxBri )
                          / (uint32_t)(WAVE_DURATION/2) );
      }
      if (val > brTmp[i]) brTmp[i] = val;
      anyOn = true;
    }
    if (!anyOn) {
      waves[w].active = false;
    }
  }

  for (int i = 0; i < 3; ++i) {
    if (brTmp[i] > ledOutput[i]) ledOutput[i] = brTmp[i];
  }
}

int overSample(int chan, int samples = 16) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(adcPins[chan]);
    delayMicroseconds(10);
  }
  return sum / samples;
}

float y[] = {0.0, 0.0, 0.0};
int expoMovAvr(int chan, float alpha = 0.05) {
  float x = analogRead(adcPins[chan]);
  y[chan] = y[chan] + alpha * (x - y[chan]);
  return y[chan];
}

void drawWrappedText(U8G2 &u8g2, int x, int y, int maxWidth, const char *text) {
  int cursorX = x;
  int cursorY = y;
  int lineHeight = u8g2.getMaxCharHeight();

  for (int i = 0; text[i] != '\0'; i++) {
    char c[2] = { text[i], '\0' };
    int charWidth = u8g2.getStrWidth(c);

    if (cursorX + charWidth > x + maxWidth) {
      cursorX = x;
      cursorY += lineHeight;
    }

    u8g2.drawStr(cursorX, cursorY, c);

    cursorX += charWidth;
  }
}

int getButton() {
  for (int i = 0; i < 4; i++) {
    if (!digitalRead(btnPins[i])) {
      if (!holding[i]) {
        pressTime[i] = millis();
        holding[i] = true;
      }
    } else {
      if (holding[i]) {
        holding[i] = false;
        return i;
      }
    }
  }
  return -1;
}

void otaUpdate() {
  static uint8_t spinnerIndex = 0;
  static bool doingOTA = true;
  const char spinnerFrames[4] = {'|', '/', '-', '\\'};
  unsigned long displayTime = millis();
  l.setBrightness(128);

  ArduinoOTA
    .onProgress([&](unsigned int progress, unsigned int total) {
      int percent = (progress * 100) / total;

      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_gulim11_t_korean1);

      if (percent == 0) {
        char frame[2] = {spinnerFrames[spinnerIndex], '\0'};
        spinnerIndex = (spinnerIndex + 1) % 4;

        u8g2.drawStr((128 - u8g2.getStrWidth("Waiting..."))/2, 20, "Waiting...");
        u8g2.drawStr((128 - u8g2.getStrWidth(frame))/2, 48, frame);
      } else {
        u8g2.drawStr((128 - u8g2.getStrWidth("Updating..."))/2, 14, "Updating...");
        u8g2.drawFrame(10, 28, 108, 12);
        int barWidth = (108 * percent) / 100;
        u8g2.drawBox(10, 28, barWidth, 12);

        String percentStr = String(percent) + "%";
        u8g2.drawStr((128 - u8g2.getStrWidth(percentStr.c_str()))/2, 54, percentStr.c_str());
      }
      if (millis() - displayTime > 1000) {
        l.fill(l.Color(255, 170, 0));
        l.show();
        u8g2.sendBuffer();
        displayTime = millis();
      }
    })
    .onEnd([]() {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_fub20_tf);
      u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      u8g2.drawStr((128 - u8g2.getStrWidth("Please Restart!"))/2, 54, "Please Restart!");
      u8g2.sendBuffer();
      l.fill(l.Color(255, 255, 255));
      l.show();
      ESP.restart();
      doingOTA = false;
    });

  ArduinoOTA.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr((128 - u8g2.getStrWidth("OTA Update"))/2, 20, "OTA Update");
  IPAddress ip = WiFi.localIP();
  String ipStr = ip.toString();

  ipStr = "IP: " + ipStr;
  u8g2.drawStr((128 - u8g2.getStrWidth(ipStr.c_str()))/2, 63, ipStr.c_str());
  u8g2.drawStr((128 - u8g2.getStrWidth("Waiting..."))/2, 36, "Waiting...");
  u8g2.sendBuffer();

  l.fill(l.Color(0, 255, 0));
  l.show();

  while (doingOTA) {
    ArduinoOTA.handle();
    int btn = getButton();
    if (btn == 3) {doingOTA = false; break;}
    delay(100);
  }
  ESP.restart();
}

#define GRAPH_WIDTH 88
#define GRAPH_HEIGHT 32

float graphData[GRAPH_WIDTH];
int graphIndex = 0;

void pushGraphValue(float val) {
  int y = (int)(val * (GRAPH_HEIGHT - 1));
  graphData[graphIndex] = y;
  graphIndex = (graphIndex + 1) % GRAPH_WIDTH;
}

void drawGraph(int x, int y) {
  u8g2.drawFrame(x, y, GRAPH_WIDTH, GRAPH_HEIGHT);

  int prevY = -1;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    int bufIndex = (graphIndex + i) % GRAPH_WIDTH;
    int valY = graphData[bufIndex];

    int drawX = x + GRAPH_WIDTH - 1 - i;
    int drawY = y + GRAPH_HEIGHT - 1 - valY;

    if (prevY >= 0) {
      u8g2.drawLine(drawX + 1, prevY, drawX, drawY);
    }
    prevY = drawY;
  }
}

void inputTypeDigitalEmulation() {
  for (int i = 0; i < 6; i++) {
    if (i < 3) {
      if (doFilter) {
        if (filterType == 0) {
          for (int i = 0; i < 3; i++)
            rawVal[i] = overSample(i);
        } else {
          for (int i = 0; i < 3; i++)
            rawVal[i] = expoMovAvr(i);
        }
      } else rawVal[i] = analogRead(adcPins[i]);
      float hall = constrain((rawVal[i] - calMin[i]) / (float)(calMax[i] - calMin[i] + 1), 0.00, 1.00);
      nowPress[i] = (hall > actuation);
      if ((nowPress[i] != lastPress[i]) && nowPress[i]) {
        applyEffect[i] = true;
      }
      hallVal[i] = hall;
    } else {
      nowPress[i] = !digitalRead(btnPins[i - 3]);
    }
    if (nowPress[i] != lastPress[i]) needReport = true;
    lastPress[i] = nowPress[i];
  }
}

void inputTypeHysteresisHandling() {
  for (int i = 0; i < 6; i++) {
    if (i < 3) {
      if (doFilter) {
        if (filterType == 0) {
          for (int i = 0; i < 3; i++)
            rawVal[i] = overSample(i);
        } else {
          for (int i = 0; i < 3; i++)
            rawVal[i] = expoMovAvr(i);
        }
      } else rawVal[i] = analogRead(adcPins[i]);
      float hall = constrain((rawVal[i] - calMin[i]) / (float)(calMax[i] - calMin[i] + 1), 0.00, 1.00);
      if (hall > upperThreshold)
        nowPress[i] = true;
      else if (hall < lowerThreshold)
        nowPress[i] = false;
      if ((nowPress[i] != lastPress[i]) && nowPress[i]) {
        applyEffect[i] = true;
      }
      hallVal[i] = hall;
    } else {
      nowPress[i] = !digitalRead(btnPins[i - 3]);
    }
    if (nowPress[i] != lastPress[i]) needReport = true;
    lastPress[i] = nowPress[i];
  }
}

void inputTypeDynamicActuation() {
  for (int i = 0; i < 6; i++) {
    if (i < 3) {
      if (doFilter) {
        if (filterType == 0) {
          for (int i = 0; i < 3; i++)
            rawVal[i] = overSample(i);
        } else {
          for (int i = 0; i < 3; i++)
            rawVal[i] = expoMovAvr(i);
        }
      } else rawVal[i] = analogRead(adcPins[i]);
      float hall = constrain((rawVal[i] - calMin[i]) / (float)(calMax[i] - calMin[i] + 1), 0.00, 1.00);
      if (hall > windowFoot[i] + windowSize) {
        nowPress[i] = true;
        windowFoot[i] = hall - windowSize;
      }
      else if (hall < windowFoot[i]) {
        nowPress[i] = false;
        windowFoot[i] = hall;
      }
      if (hall == 0.0) windowFoot[i] = 0.0;
      if (hall == 1.0) windowFoot[i] = 1.0 - windowSize;
      if ((nowPress[i] != lastPress[i]) && nowPress[i]) {
        applyEffect[i] = true;
      }
      hallVal[i] = hall;
    } else {
      nowPress[i] = !digitalRead(btnPins[i - 3]);
    }
    if (nowPress[i] != lastPress[i]) needReport = true;
    lastPress[i] = nowPress[i];
  }
}

void updateInput() {
  switch (inputHandler) {
    case 0: inputTypeDigitalEmulation(); break;
    case 1: inputTypeHysteresisHandling(); break;
    case 2: inputTypeDynamicActuation(); break;
    default: inputTypeDigitalEmulation(); break;
  }
}

void screenSaver(const char* title) {
  u8g2.clearBuffer();
  if (logoType > 1 && logoType < 12) {
    u8g2.drawXBMP(20, 20, 88, 232, logoKao[logoType - 2]);
    u8g2.setDrawColor(0);
    u8g2.drawBox(20, 0, 88, 20);
    u8g2.drawBox(20, 40, 88, 24);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 54, title);
  }
  else {
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.drawStr((128 - u8g2.getStrWidth(screenLogo.c_str()))/2, 40, screenLogo.c_str());
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 54, title);
  }
  u8g2.sendBuffer();
}

void calibMenu() {
  bool running = true;
  bool calib = false;
  int nowCal = 0;
  ledcWrite(0, 2);
  for (int i = 0;  i < GRAPH_WIDTH; i++) graphData[i] = {0};
  while (running) {
    updateInput();
    if (calib) {
      calMin[nowCal] = (calMin[nowCal] < rawVal[nowCal] + deadZone) ? calMin[nowCal] : rawVal[nowCal] + deadZone;
      calMax[nowCal] = (calMax[nowCal] > rawVal[nowCal] - deadZone) ? calMax[nowCal] : rawVal[nowCal] - deadZone;
    }
    switch (getButton())
    {
    case 0:
      nowCal = (nowCal + 2) % 3;
      for (int i = 0;  i < GRAPH_WIDTH; i++)
        graphData[i] = {0};
      for (int i = 0; i < 3; i++) {
        if (i == nowCal)
          ledcWrite(i, 2);
        else
          ledcWrite(i, 0);
      }
      if (calib) {
        calMin[nowCal] = 4095;
        calMax[nowCal] = 0;
      }
      break;
    case 1:
      calib = !calib;
      if (calib) {
        calMin[nowCal] = 4095;
        calMax[nowCal] = 0;
      }
      break;
    case 2:
      nowCal = (nowCal + 1) % 3;
      for (int i = 0;  i < GRAPH_WIDTH; i++)
        graphData[i] = {0};
      for (int i = 0; i < 3; i++) {
        if (i == nowCal)
          ledcWrite(i, 2);
        else
          ledcWrite(i, 0);
      }
      if (calib) {
        calMin[nowCal] = 4095;
        calMax[nowCal] = 0;
      }
      break;
    case 3:
      running = false;
    default:
      break;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr(0, 12, ((calib ? "Calibrating S" : "Viewing S") + String(nowCal + 1)).c_str());
    u8g2.drawStr(0, 24, ("+ " + String((int)calMax[nowCal])).c_str());
    u8g2.drawStr(0, 35, ("p " + String((int)rawVal[nowCal])).c_str());
    u8g2.drawStr(0, 46, ("- " + String((int)calMin[nowCal])).c_str());
    u8g2.setFont(u8g2_font_5x8_mf);
    u8g2.drawStr(108, 12, String(hallVal[nowCal]).c_str());
    u8g2.drawStr(0, 56, "F1: Last"); u8g2.drawStr(50, 56, calib ? "F2: Stop Calib" : "F2: Calibrate");
    u8g2.drawStr(0, 64, "F3: Next"); u8g2.drawStr(50, 64, "F4: Exit");
    pushGraphValue(hallVal[nowCal]);
    drawGraph(40, 14);
    if (calib)
      u8g2.drawLine(40, (int)((1.0 - hallVal[nowCal]) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - hallVal[nowCal]) * (GRAPH_HEIGHT - 1)) + 14);
    else
      switch (inputHandler) {
        case 0: u8g2.drawLine(40, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14); break;
        case 1:
          u8g2.drawLine(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        case 2:
          u8g2.drawLine(40, (int)((1.0 - windowFoot[nowCal]) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - windowFoot[nowCal]) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - (windowFoot[nowCal] + windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (windowFoot[nowCal] + windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        default: break;
      }
    u8g2.sendBuffer();
  }
  for (int i = 0; i < 3; i++) {
    ledcWrite(i, 0);
  }
}

void inputMenu() {
  bool running = true;
  int hysteresisChange = 0;
  bool hysteresisChanging = false;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    graphData[i] = 0;
  }
  while (running) {
    u8g2.clearBuffer();
    float maxPress = max(max(hallVal[0], hallVal[1]), hallVal[2]);
    float maxFoot = max(max(windowFoot[0], windowFoot[1]), windowFoot[2]);
    pushGraphValue(maxPress);
    drawGraph(40, 14);
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    switch (inputHandler) {
      case 0:
      inputTypeDigitalEmulation();
        u8g2.drawStr(0, 12, "DigitalEmulation");
        u8g2.drawStr(0, 24, "Acc:");
        u8g2.drawStr(0, 36, String(actuation).c_str());
        u8g2.drawLine(40, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14);
        break;
      case 1:
        inputTypeHysteresisHandling();
        u8g2.drawStr(0, 12, "Hysteresis");
        u8g2.drawStr(0, 24, ("U: " + String(upperThreshold)).c_str());
        u8g2.drawStr(0, 36, ("L: " + String(lowerThreshold)).c_str());
        u8g2.drawLine(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        u8g2.drawLine(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        if (hysteresisChange == 0) {
          u8g2.drawTriangle(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 10, 40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 18, 46, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        } else {
          u8g2.drawTriangle(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 10, 40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 18, 46, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        }
        break;
      case 2:
        inputTypeDynamicActuation();
        u8g2.drawStr(0, 12, "DynamicAct");
        u8g2.drawStr(0, 24, ("W: " + String(windowSize)).c_str());
        u8g2.drawStr(0, 36, ("F: " + String(maxFoot)).c_str());
        u8g2.drawLine(40, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14, 128  , (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14);
        u8g2.drawLine(40, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
        break;
      default:
        inputTypeDigitalEmulation();
        break;
    }
    u8g2.setFont(u8g2_font_5x8_mf);
    u8g2.drawStr(108, 12, String(maxPress).c_str());
    switch (inputHandler) {
      case 0:
        u8g2.drawStr(0, 56, "F1: Acc+    F2: Change M"); // not enough space
        u8g2.drawStr(0, 64, "F3: Acc-    F4: Exit");
        break;
      case 1:
        if (!hysteresisChanging) {
          u8g2.drawStr(0, 56, "F1: Next    F2: Change M");
          u8g2.drawStr(0, 64, "F3: Change  F4: Exit");
        }
        else if (hysteresisChange == 0) {
          u8g2.drawStr(0, 56, "F1: U+      F2: Done");
          u8g2.drawStr(0, 64, "F1: U-      F4: Exit");
        }
        else if (hysteresisChange == 1) {
          u8g2.drawStr(0, 56, "F1: L+      F2: Done");
          u8g2.drawStr(0, 64, "F3: L-      F4: Exit");
        }
        break;
      case 2:
        u8g2.drawStr(0, 56, "F1: Ws+     F2: Change M");
        u8g2.drawStr(0, 64, "F3: Ws-     F4: Exit");
        break;
    }
    switch (getButton()) {
      case 0:
        if (inputHandler == 0) {
          actuation += 0.05;
          actuation = constrain(actuation, 0.05, 0.95);
        } else if (inputHandler == 1) {
          if (!hysteresisChanging) {
            hysteresisChange = (hysteresisChange + 1) % 2;
          }
          else if (hysteresisChange == 0) {
            upperThreshold += 0.05;
            upperThreshold = constrain(upperThreshold, lowerThreshold + 0.05, 0.95);
          }
          else if (hysteresisChange == 1) {
            lowerThreshold += 0.05;
            lowerThreshold = constrain(lowerThreshold, 0.05, upperThreshold - 0.05);
          }
        } else if (inputHandler == 2) {
          windowSize += 0.05;
          windowSize = constrain(windowSize, 0.05, 1.00);
        }
        break;
      case 1:
        if (!(hysteresisChanging && inputHandler == 1))
          inputHandler = (inputHandler + 1) % 3;
        else
          hysteresisChanging = false;
        break;
      case 2:
        if (inputHandler == 0) {
          actuation -= 0.05;
          actuation = constrain(actuation, 0.05, 0.95);
        } else if (inputHandler == 1) {
          if (!hysteresisChanging) {
            hysteresisChanging = true;
          }
          else if (hysteresisChange == 0) {
            upperThreshold -= 0.05;
            upperThreshold = constrain(upperThreshold, lowerThreshold + 0.05, 0.95);
          }
          else if (hysteresisChange == 1) {
            lowerThreshold -= 0.05;
            lowerThreshold = constrain(lowerThreshold, 0.05, upperThreshold - 0.05);
          }
        } else if (inputHandler == 2) {
          windowSize -= 0.05;
          windowSize = constrain(windowSize, 0.05, 1.00);
        }
        break;
      case 3:
        running = false;
      default:
        break;
    }
    u8g2.sendBuffer();
  }
}

void filtMenu() {
  bool running = true;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    graphData[i] = 0;
  }
  while (running) {
    updateInput();
    float maxHall = max(max(hallVal[0], hallVal[1]), hallVal[2]);
    float maxFoot = max(max(windowFoot[0], windowFoot[1]), windowFoot[2]);
    int btn = getButton();
    switch (btn) {
      case 0: filterType = 0; break;
      case 1: doFilter = !doFilter; break;
      case 2: filterType = 1; break;
      case 3: running = false; break;
      default: break;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr(0, 12, doFilter ? "Filter" : "Filter [Off]");
    u8g2.drawStr(0, 24, ("1 " + String((int)rawVal[0])).c_str());
    u8g2.drawStr(0, 35, ("2 " + String((int)rawVal[1])).c_str());
    u8g2.drawStr(0, 46, ("3 " + String((int)rawVal[2])).c_str());
    u8g2.setFont(u8g2_font_5x8_mf);
    u8g2.drawStr(108, 12, String(maxHall).c_str());
    u8g2.drawStr(0, 56, filterType == 0 ? "> F1: OV" : "F1: OV"); u8g2.drawStr(50, 56, doFilter ? "F2: Disable" : "F2: Enable");
    u8g2.drawStr(0, 64, filterType == 1 ? "> F3: AV" : "F3: AV"); u8g2.drawStr(50, 64, "F4: Exit");
    pushGraphValue(maxHall);
    drawGraph(40, 14);
      switch (inputHandler) {
        case 0: u8g2.drawLine(40, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14); break;
        case 1:
          u8g2.drawLine(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        case 2:
          u8g2.drawLine(40, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        default: break;
      }
    u8g2.sendBuffer();
  }
  if (doFilter) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    int curious = u8g2.userInterfaceMessage(
      "ADC may change",
      "slower while display",
      "is working",
      " Ok \n More "
    ); // for later info
  }
}

float smtLed[] = {0.00, 0.00, 0.00};
void udgSmooth() {
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();
  for (int i = 0; i < 3; i++) {
    smtLed[i] += ((nowPress[i] ? 1.00 : 0.00) - smtLed[i]) * 0.32;
    int tmpOutput = (int)(smtLed[i] * maxBri);
    ledcWrite(i, tmpOutput);
  }
}

float burnLevel[] = {0.00, 0.00, 0.00};
unsigned long lastPressT[] = {0, 0, 0};
int decayTime = 5000;
int spamReq = 125; // ~8 CPS, if slower then ignore the click and not burn
void udgBurnIn() {
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();
  for (int i = 0; i < 3; i++) {
    if (applyEffect[i]) {
      if (millis() - lastPressT[i] < spamReq)
        burnLevel[i] = constrain(burnLevel[i] + 0.01, 0.00, 1.00);
      applyEffect[i] = false;
      lastPressT[i] = millis();
    }
    if (millis() - lastPressT[i] > decayTime) {
      burnLevel[i] = constrain(burnLevel[i] - 0.001, 0.00, 1.00);
    }
    smtLed[i] += ((nowPress[i] ? 1.00 : 0.00) - smtLed[i]) * 0.32;
    int tmpOutput = (int)(burnLevel[i] * 255 > smtLed[i] * maxBri ? burnLevel[i] * 255 : smtLed[i] * maxBri); // BURN YOUR EYES
    ledcWrite(i, tmpOutput);
  }
}

void udgAnalog() {
  for (int i = 0; i < 3; i++) {
    int tmpOutput = (int)(hallVal[i] * maxBri);
    ledcWrite(i, tmpOutput);
  }
}

void udgSoild() {
  for (int i = 0; i < 3; i++) {
    int tmpOutput = nowPress[i] ? maxBri : 0;
    ledcWrite(i, tmpOutput);
  }
}

void effectMenu() {
  int sel = 1;
  while (sel != 0) {
    String menu_items =
      "Under Glow " + String(underGlow ? "[On]" : "[Off]") + "\n"
      + "RGB Led " + String(rgb ? "[On]" : "[Off]");
    sel = u8g2.userInterfaceSelectionList("Effects", sel, menu_items.c_str());
    if (sel == 1) {
      int subSel = 1;
      String effectsList[] = {
        "Tap",
        "Ripple",
        "Smooth",
        "Burn-in",
        "Analog",
        "Soild"
      };
      while (subSel != 0) {
        String tmp =
        "Under Glow: " + String(underGlow ? "[On]" : "[Off]") + "\n"
        + "Type: " + effectsList[glowType] + "\n";
        for (int i = 0; i < 6; i++) {
          tmp += (glowType == i ? "> " : "") + effectsList[i] + (glowType == i ? " <" : "");
          if (i < 5) tmp += "\n";
        }
        subSel = u8g2.userInterfaceSelectionList("Under Glow", subSel, tmp.c_str());
        if (subSel == 1) underGlow = !underGlow;
        else if (subSel >= 3 && subSel <= 8) glowType = subSel - 3;
      }
    } else if (sel == 2) {
      int subSel = 1;
      while (subSel != 0){
        String menuElements =
          "RGB Led: " + String(rgb ? "[On]" : "[Off]") + "\n"
          + "Brightness: " + String(rgbBri) + "\n"
          + "Mode: " + String(doRainbow ? "Rainbow" : "Static") + "\n";
        if (doRainbow) {
          menuElements += "Speed: " + String(rainbowStep) + "\n";
          menuElements += "Interval: " + String(rgbInterval * 10) + " ms";
        } else {
          menuElements += "Color (" + String(color[0]) + ", " + String(color[1]) + ", " + String(color[2]) + ")";
        }
        subSel = u8g2.userInterfaceSelectionList("RGB Led", subSel, menuElements.c_str());
        if (subSel == 1) rgb = !rgb;
        if (subSel == 2) u8g2.userInterfaceInputValue("Brightness\n", "(0 - 255): ", &rgbBri, 0, 255, 3, " ");
        if (subSel == 3) doRainbow = !doRainbow;
        if (subSel == 4) {
          if (doRainbow) {
            u8g2.userInterfaceInputValue("Speed\n", "(1 - 255): ", &rainbowStep, 1, 255, 3, " ");
          } else {
            int sub2Sel = 1;
            while (sub2Sel != 0) {
              String colorElements =
                + "R: " + String(color[0]) + "\n"
                + "G: " + String(color[1]) + "\n"
                + "B: " + String(color[2]);
              sub2Sel = u8g2.userInterfaceSelectionList("Color\n", sub2Sel, colorElements.c_str());
              if (sub2Sel == 1) u8g2.userInterfaceInputValue("Red\n", "(0 - 255): ", &color[0], 0, 255, 3, " ");
              else if (sub2Sel == 2) u8g2.userInterfaceInputValue("Blue\n", "(0 - 255): ", &color[1], 0, 255, 3, " ");
              else if (sub2Sel == 3) u8g2.userInterfaceInputValue("Green\n", "(0 - 255): ", &color[2], 0, 255, 3, " ");
            }
          }
        }
        if (subSel == 5) u8g2.userInterfaceInputValue("Interval\n(10ms - 2550ms)\n", "(n x 10ms): ", &rgbInterval, 1, 255, 3, "n");
      }
    }
  }
}

// very unnecessary important code for morse code input, because who tf REMEMBERS THE WHOLE FUCKING MORSE CODE

struct MorseNode {
  char letter;
  MorseNode *dot;
  MorseNode *dash;
};

MorseNode root = {'\0', nullptr, nullptr};

void addMorse(const char *code, char letter) {
  MorseNode *node = &root;
  for (const char *p = code; *p; p++) {
    if (*p == '.') {
      if (!node->dot) node->dot = new MorseNode{'\0', nullptr, nullptr};
      node = node->dot;
    } else if (*p == '-') {
      if (!node->dash) node->dash = new MorseNode{'\0', nullptr, nullptr};
      node = node->dash;
    }
  }
  node->letter = letter;
}

bool unsignedCharacter = false;
char decodeMorse(const char *code) {
  MorseNode *node = &root;
  for (const char *p = code; *p; p++) {
    unsignedCharacter = false;
    if (*p == '.' && node->dot) node = node->dot;
    else if (*p == '-' && node->dash) node = node->dash;
    else {unsignedCharacter = true; return '?';}
  }
  return node->letter ? node->letter : '?';
}

bool genMorse = false;
void setupMorse() {
  addMorse(".-", 'a');
  addMorse("-...", 'b');
  addMorse("-.-.", 'c');
  addMorse("-..", 'd');
  addMorse(".", 'e');
  addMorse("..-.", 'f');
  addMorse("--.", 'g');
  addMorse("....", 'h');
  addMorse("..", 'i');
  addMorse(".---", 'j');
  addMorse("-.-", 'k');
  addMorse(".-..", 'l');
  addMorse("--", 'm');
  addMorse("-.", 'n');
  addMorse("---", 'o');
  addMorse(".--.", 'p');
  addMorse("--.-", 'q');
  addMorse(".-.", 'r');
  addMorse("...", 's');
  addMorse("-", 't');
  addMorse("..-", 'u');
  addMorse("...-", 'v');
  addMorse(".--", 'w');
  addMorse("-..-", 'x');
  addMorse("-.--", 'y');
  addMorse("--..", 'z');
  addMorse("-----", '0');
  addMorse(".----", '1');
  addMorse("..---", '2');
  addMorse("...--", '3');
  addMorse("....-", '4');
  addMorse(".....", '5');
  addMorse("-....", '6');
  addMorse("--...", '7');
  addMorse("---..", '8');
  addMorse("----.", '9');
  addMorse(".-.-.-", '.');
  addMorse("--..--", ',');
  addMorse("..--..", '?');
  addMorse(".----.", '\'');
  addMorse("-.-.--", '!');
  addMorse("-..-.", '/');
  addMorse("-.--.", '(');
  addMorse("-.--.-", ')');
  addMorse(".-...", '&');
  addMorse("---...", ':');
  addMorse("-.-.-.", ';');
  addMorse("-...-", '=');
  addMorse(".-.-.", '+');
  addMorse("-....-", '-');
  addMorse("..--.-", '_');
  addMorse(".--.-.", '@');
  addMorse(".-..-.", '\"');
  addMorse("...-.", '*');
  addMorse("-.-.-", '\\');
  addMorse("---.-", '%');
  addMorse("--.-.", '#');
  addMorse("--.-.-", '|');
  addMorse("......", '^');
  addMorse(".---..", '~');
  addMorse("-..-.-", '`');
  addMorse("...-..", '$');
  addMorse(".--..", '[');
  addMorse(".--..-", ']');
  addMorse(".--.-", '{');
  addMorse(".--.--", '}');
  addMorse("-.---", '<');
  addMorse("-.----", '>');
  addMorse("..--", ' ');
}

bool morseMode = true;
String keyboard(String text) {
  String prevText = text;
  if (morseMode) {
    bool capsLock = false;
    setupMorse();
    bool typing = true;
    String currentCode = "";
    unsigned long lastInputTime = millis();
    while (typing) {
      updateInput();
      if (applyEffect[0]) { // Dot
        currentCode += currentCode.length() < 8 ? "." : "";
        lastInputTime = millis();
        applyEffect[0] = false;
      }
      if (applyEffect[1]) { // Dash
        currentCode += currentCode.length() < 8 ? "-" : "";
        lastInputTime = millis();
        applyEffect[1] = false;
      }
      if (applyEffect[2]) { // Interrupt
        lastInputTime = millis() + 1200;
        applyEffect[2] = false;
      }
      if (millis() - lastInputTime > 800 && currentCode.length() > 0) {
        if (currentCode == ".-.-") typing = false; // Enter
        else if (currentCode == "----") text.remove(text.length() - 1); // Backspace
        else if (currentCode == "....-.") capsLock = !capsLock; // Caps Lock
        else {
          char decodedChar = decodeMorse(currentCode.c_str());
          if (text.length() < 255 && !unsignedCharacter)
            text += (char)(capsLock ? toupper(decodedChar) : decodedChar);
        }
        currentCode = "";
      }
      int button = getButton();
      if (button == 0) {
        if (millis() - pressTime[0] < 1000) text.remove(text.length() - 1);
        else text = "";
      }
      if (button == 1) capsLock = !capsLock;
      if (button == 2) {
        int opt = u8g2.userInterfaceMessage(
          "Morse Code Keyboard",
          "using GBoard Morse",
          "code layout",
          " OK \n Hint \n QR "
        );
        // leave for now
      }
      if (button == 3) return prevText;
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_fub20_tf);
      u8g2.drawStr(64 - u8g2.getStrWidth(currentCode.c_str()) / 2, 20, currentCode.c_str());
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      char tmp = decodeMorse(currentCode.c_str());
      String tmpStr = unsignedCharacter ? "[???]" : String(tmp);
      if (currentCode == "..--") tmpStr = "[space]";
      else if (currentCode == ".-.-") tmpStr = "[enter]";
      else if (currentCode == "----") tmpStr = "[backspace]";
      else if (currentCode == "....-.") tmpStr = "[caps " + String(capsLock ? "off" : "on") + "]";
      if (currentCode != "") u8g2.drawStr(64 - u8g2.getStrWidth(tmpStr.c_str()) / 2, 32, tmpStr.c_str());
      else u8g2.drawStr(64 - u8g2.getStrWidth("[none]") / 2, 32, "[none]");
      u8g2.drawStr(0, 40, String((String)"Text: " + (capsLock ? "[C]" : "")).c_str());
      drawWrappedText(u8g2, 0, 50, 128, (((millis() - lastInputTime) % 500 < 250) ? text : text + "_").c_str());
      u8g2.sendBuffer();
    }
  }
  return text;
  root = {'\0', nullptr, nullptr}; // destroy the tree to save memory
}

void mgp() {
  l.setBrightness(255);
  BleGamepad g(btName.c_str(), "NoID", 100);
  BleGamepadConfiguration cfg;
  cfg.setAutoReport(false);
  g.begin(&cfg);
  bool running = true;
  bool enableThumbs = false;
  unsigned long lps = millis();
  waitIDLE = millis();
  screenWait = false;
  screenOff = false;
  while (running) {
    updateInput();
    mpu.update();
    int normLx = (int)((hallVal[2] - hallVal[1] + 1.0f) * 16383.5f); // [-1..1] → [0..32767]
    int normLy = (int)((1.00f - hallVal[0]) * 32767.0f);                       // [0..1] → [0..32767]
    int gRx    = 32767 - (int)((constrain(mpu.getAngleY(), -70, 70) + 70) * 234); // [-90..90] → [0..32767]
    int gRy    = 32767 - (int)((constrain(mpu.getAngleX(), -70, 70) + 70) * 234);
    int button = getButton();
    if (nowPress[3]) g.press(1); else g.release(1); // A
    if (nowPress[4]) g.press(2); else g.release(2); // B
    if (button == 2) {
      if (millis() - pressTime[2] < 1000)
        enableThumbs = (screenOff || screenWait) ? enableThumbs : !enableThumbs;
      else {
        u8g2.clearBuffer();
        u8g2.drawStr(64 - u8g2.getStrWidth("Calibrating") / 2, 32, "Calibrating...");
        u8g2.sendBuffer();
        mpu.calcOffsets();
        delay(200);
      }
      waitIDLE = millis();
      screenOff = false;
      screenWait = false;
      u8g2.setPowerSave(0);
    }
    if (button == 3) running = false; // Exit
    if (!enableThumbs) {normLx = 16383; normLy = 16383; gRx = 16383; gRy = 16383;}
    g.setLeftThumb(normLx, normLy);
    g.setRightThumb(gRx, gRy);
    g.sendReport();
    if (millis() - waitIDLE < screenSaveDuration) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_5x8_mf);
      u8g2.drawStr(0, 12, "BLE Mode [Gamepad]");
      l.fill(l.Color(0, 255, 255));
      if (!g.isConnected()) {u8g2.drawStr(0, 20, "Not Connected"); l.fill(l.Color(255, 0, 0));}
      else if (!enableThumbs) {u8g2.drawStr(0, 20, "Thumbs Disabled"); l .fill(l.Color(255, 255, 0));}
      else {
        String tmp = "rx:" + String(gRx) + ", ry:" + String(gRy);
        u8g2.drawStr(0, 20, tmp.c_str());
      }
      u8g2.drawFrame(4, 26, 32, 32);
      int cputmp = temperatureRead();
      String tmp = String(cputmp) + String((char)(0xB0)) + "C";
      u8g2.drawStr(64 - u8g2.getStrWidth(tmp.c_str()) / 2, 42, tmp.c_str());
      u8g2.drawFrame(92, 26, 32, 32);
      if (enableThumbs) {
        int cLx = map(normLx, 0, 32767, 6, 34);
        int cLy = map(normLy, 0, 32767, 28, 56);
        u8g2.drawCircle(cLx, cLy, 2, U8G2_DRAW_ALL);
        int cRx = map(gRx, 0, 32767, 96, 122);
        int cRy = map(gRy, 0, 32767, 28, 56);
        u8g2.drawCircle(cRx, cRy, 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawCircle(18, 42, 2, U8G2_DRAW_ALL);
        u8g2.drawCircle(106, 42, 2, U8G2_DRAW_ALL);
      }
      u8g2.sendBuffer();
      l.show();
    }
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("BLE Gamepad");
        screenWait = true;
        l.fill(l.Color(0, 0, 255));
        l.show();
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
        l.fill(l.Color(255, 0, 255));
        l.show();
      }
    }
    if (millis() - lps < 16) delay(millis() - lps);
    else lps = millis();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(64 - u8g2.getStrWidth("Exiting...") / 2, 32, "Exiting...");
  u8g2.sendBuffer();
  g.end();
  BLEDevice::deinit(true);
  delay(1000);
  l.setBrightness(0);
  l.show();
  u8g2.setPowerSave(0);
}

void marm() {
  l.setBrightness(255);
  BleMouse m(btName.c_str(), "NoID", 100);
  m.begin();
  bool running = true;
  bool enableMove = false;
  unsigned long lps = millis();
  waitIDLE = millis();
  screenWait = false;
  screenOff = false;
  while (running) {
    updateInput();
    mpu.update();
    int mvx = constrain(-mpu.getGyroZ() / 5, -10, 10);
    int mvy = constrain(mpu.getGyroX()  / 5, -10, 10);
    int button = getButton();
    int scroll = 0;
    if (nowPress[0]) m.press(MOUSE_LEFT); else m.release(MOUSE_LEFT);
    if (nowPress[1]) m.press(MOUSE_MIDDLE); else m.release(MOUSE_MIDDLE);
    if (nowPress[2]) m.press(MOUSE_RIGHT); else m.release(MOUSE_RIGHT);
    if (nowPress[3]) scroll = 1; // Up
    else if (nowPress[4]) scroll = -1; // Down
    if (enableMove) m.move(mvx, mvy, scroll);
    else if (scroll != 0) m.move(0, 0, scroll);
    if (button == 2) {
      if (millis() - pressTime[2] < 1000)
        enableMove = (screenOff || screenWait) ? enableMove : !enableMove;
      else {
        u8g2.clearBuffer();
        u8g2.drawStr(64 - u8g2.getStrWidth("Calibrating") / 2, 32, "Calibrating...");
        u8g2.sendBuffer();
        mpu.calcOffsets();
        delay(200);
      }
      waitIDLE = millis();
      screenOff = false;
      screenWait = false;
      u8g2.setPowerSave(0);
    }
    if (button == 3) running = false;
    if (millis() - waitIDLE < screenSaveDuration) {
      u8g2.clearBuffer();
      l.fill(l.Color(0, 255, 255));
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      u8g2.drawStr((128 - u8g2.getStrWidth("BLE Mouse"))/2, 24, "BLE Mouse");
      String tmp = "Temp: " + String((int)temperatureRead()) + String((char)(0xB0)) + "C";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 48, tmp.c_str());
      tmp = "gx:" + String((int)mpu.getGyroX()) + ", gz:" + String((int)mpu.getGyroZ());
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 12, tmp.c_str());
      if (!m.isConnected()) {
        u8g2.drawStr((128 - u8g2.getStrWidth("Not Connected"))/2, 36, "Not Connected");
        l.fill(l.Color(255, 120, 0));
      } else if (!enableMove) {
        u8g2.drawStr((128 - u8g2.getStrWidth("Gyro Disabled"))/2, 36, "Gyro Disabled");
        l.fill(l.Color(255, 120, 0));
      }
      l.show();
      u8g2.sendBuffer();
    }
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("BLE Mouse");
        screenWait = true;
        l.fill(l.Color(0, 0, 255));
        l.show();
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
        l.fill(l.Color(255, 0, 255));
        l.show();
      }
    }
    if (millis() - lps < 32) delay(millis() - lps);
    else lps = millis();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(64 - u8g2.getStrWidth("Exiting...") / 2, 32, "Exiting...");
  u8g2.sendBuffer();
  m.end();
  BLEDevice::deinit(true);
  delay(1000);
  l.setBrightness(0);
  l.show();
}

int randRange(int min, int max) {return min + esp_random() % (max - min + 1);}

uint8_t charToKey(char c, bool &shift) {
  shift = false;
  if (c >= 'a' && c <= 'z') {
    return HID_KEY_A + (c - 'a');   // a..z
  }
  if (c >= 'A' && c <= 'Z') {
    shift = true;
    return HID_KEY_A + (c - 'A');   // A..Z (shift)
  }
  if (c >= '1' && c <= '9') {
    return HID_KEY_1 + (c - '1');   // 1..9
  }
  if (c == '0') {
    return HID_KEY_0;                // 0
  }

  if (c = ' ') return HID_KEY_SPACE;
  return 0; // unsupported
}

// short story: NEVER trust bluetooth libraries
// but i use raw reporting for almost all of them (usb, ble mouse, ble keyboard, gamepad is good though)
// BECAUSE ALL OF THEM SUCKS
// like, bro, just think about it, who tf genius point the BleKeyboard::print() to some mf system print that i have no fking ideal???

void mwk() {
  l.setBrightness(255);
  BleKeyboard kb(btName.c_str(), "NoID", 100);
  kb.begin();
  bool running = true;
  waitIDLE = millis();
  screenWait = false;
  screenOff = false;
  while (running) {
    updateInput();
    // Report
    if (kb.isConnected()) {
      if (needReport) {
        KeyReport report = {0};
        report.modifiers = 0;
        uint8_t idx = 0;
        for (int i = 0; i < 6; i++)
          if (nowPress[i]) report.keys[idx++] = layout[i];
        kb.sendReport(&report);
      }
    }
    bool button = getButton() == 3;
    if (button) {
      if (millis() - pressTime[3] > 1000) running = false;
      else {
        if (screenOff) u8g2.setPowerSave(0);
        screenWait = false;
        screenOff = false;
        waitIDLE = millis();
      }
    }
    mpu.update();
    if (abs(mpu.getAngleY()) > 80 && button) {
      if (screenOff) u8g2.setPowerSave(0);
      screenWait = false;
      screenOff = false;
      waitIDLE = millis();
      applyEffect[0] = false;
      applyEffect[1] = false;
      applyEffect[2] = false;
      String text = keyboard("");
      int lineCount = 0;
      u8g2.clearBuffer();
      if (text != "") {
        for (int i = 0; i < text.length(); i++) {
          bool shift;
          uint8_t key = charToKey((char)text[i], shift);
          if (key) {
            KeyReport report = {0};
            report.modifiers = shift ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
            report.keys[0] = key;
            kb.sendReport(&report);
            delay(randRange(10, 80));
            report.modifiers = 0;
            report.keys[0] = 0;
            kb.sendReport(&report);
            String tmpText = String(i) + ". char: "
            + String(char(text[i])) + ", code: 0x" + String(key, HEX);
            u8g2.setDrawColor(0);
            u8g2.drawBox(0, lineCount * 10, 128, 10);
            u8g2.setDrawColor(1);
            u8g2.drawStr(0, 10 + lineCount * 10, tmpText.c_str());
            u8g2.sendBuffer();
            lineCount = (lineCount + 1) % 6;
            delay(randRange(10, 80));
          }
        }
      }
      if (screenOff) u8g2.setPowerSave(0);
      screenWait = false;
      screenOff = false;
      waitIDLE = millis();
      mpu.update();
    }
    if (millis() - waitIDLE < screenSaveDuration) {
      u8g2.clearBuffer();
      l.fill(l.Color(0, 255, 255));
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      u8g2.drawStr((128 - u8g2.getStrWidth("BLE Keyboard"))/2, 24, "BLE Keyboard");
      String tmp = "Temp: " + String((int)temperatureRead()) + String((char)(0xB0)) + "C";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 48, tmp.c_str());
      if (!kb.isConnected()) {
        u8g2.drawStr((128 - u8g2.getStrWidth("Not Connected"))/2, 36, "Not Connected");
        l.fill(l.Color(255, 120, 0));
      }
      l.show();
      u8g2.sendBuffer();
    }
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("BLE Keyboard");
        screenWait = true;
        l.fill(l.Color(0, 0, 255));
        l.show();
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
        l.fill(l.Color(255, 0, 255));
        l.show();
      }
    }
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(64 - u8g2.getStrWidth("Exiting...") / 2, 32, "Exiting...");
  u8g2.sendBuffer();
  kb.end();
  BLEDevice::deinit(true);
  delay(1000);
  l.setBrightness(0);
  l.show();
}

void wifiConnectScreen() { // separate function to save flash
  unsigned long beginTime = millis();
  while (millis() - beginTime < 60000 && WiFi.status() != WL_CONNECTED) {
    int btnPress = getButton();
    if (btnPress == 3) beginTime = millis() - 62000;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 34, "Mufuki");
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    String conDot = "Connecting";
    for (int i = 0; i < (int)(millis() / 500 % 4); i++) conDot += ".";
    u8g2.drawStr((128 - u8g2.getStrWidth(conDot.c_str()))/2, 48, conDot.c_str());
    if (millis() - beginTime > 5000) {
      u8g2.setFont(u8g2_font_5x8_mf);
      String tmp = "C" + String((int)WiFi.status()) + ", " + String((millis() - beginTime) / 1000) + "s/60s, F4 to abort";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 62, tmp.c_str());
    }
    u8g2.sendBuffer();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub20_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 34, "Mufuki");
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  switch (WiFi.status()) {
    case WL_CONNECTED:
      u8g2.drawStr((128 - u8g2.getStrWidth("Connected"))/2, 48, "Connected");
    break;
    default:
      u8g2.drawStr((128 - u8g2.getStrWidth("Not connected"))/2, 48, "Not connected");
    break;
  }
  u8g2.sendBuffer();
  delay(2000);
}

void wifiMenu() {
  int subSel = 1;
  int wifiCount = -1;
  while (subSel != 0) {
    String wItems = "";
    bool wStatus = (WiFi.status() == WL_CONNECTED);
    bool wDisable = !(WiFi.getMode() != WIFI_OFF);
    if (wDisable) wItems += "Enable WiFi: [OFF]";
    else if (wStatus) {
      wItems += "Enable WiFi: [ON]\n";
      wItems += "Status: [Connected]\n";
      wItems += "IP: " + WiFi.localIP().toString() + "\n";
      wItems += "SSID: " + WiFi.SSID() + "\n";
      wItems += "Strength: " + String(map(constrain(WiFi.RSSI(), -100, -10), -100, -10, 0, 100)) + "% (" + String(WiFi.RSSI()) + ")\n";
      wItems += "Disconnect";
    } else {
      wItems += "Enable WiFi: [ON]\n";
      wItems += "Status: [Offline]\n";
      wItems += "Scan for networks\n";
      wItems += "Manual Connect";
    }
    subSel = u8g2.userInterfaceSelectionList("WiFi Settings", subSel, wItems.c_str());
    if (subSel == 1) {
      if (wDisable) {WiFi.mode(WIFI_STA); WiFi.begin();}
      else WiFi.mode(WIFI_OFF);
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Applying...") / 2, 32, "Applying...");
      u8g2.sendBuffer();
      delay(500);
    }
    if (subSel == 2) {
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Refreshing...") / 2, 32, "Refreshing...");
      u8g2.sendBuffer();
      delay(200);
    }
    if (subSel == 3) {
      if (!wStatus) { // do nothing with the wifi ip
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_fub20_tf);
        u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
        u8g2.setFont(u8g2_font_gulim11_t_korean1);
        u8g2.drawStr((128 - u8g2.getStrWidth("Searching..."))/2, 54, "Searching...");
        u8g2.sendBuffer();
        wifiCount = WiFi.scanNetworks();
        if (wifiCount == 0) {
          u8g2.userInterfaceMessage("No Networks", "Found!", "", " Ok ");
        }
        else {
          int wsSel = 1;
          while (wsSel != 0) {
            String Wlist = "Rescan\n";
            for (int i = 0; i < wifiCount; i++) {
              Wlist += String(i + 1) + ". " + WiFi.SSID(i) + " (" + String(map(constrain(WiFi.RSSI(i), -100, -10), -100, -10, 0, 100)) + "%)" + (i < wifiCount - 1 ? "\n" : "");
            }
            wsSel = u8g2.userInterfaceSelectionList("Select Network", wsSel, Wlist.c_str());
            if (wsSel == 1) {
              u8g2.clearBuffer();
              u8g2.setFont(u8g2_font_fub20_tf);
              u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
              u8g2.setFont(u8g2_font_gulim11_t_korean1);
              u8g2.drawStr((128 - u8g2.getStrWidth("Searching..."))/2, 54, "Searching...");
              u8g2.sendBuffer();
              wifiCount = WiFi.scanNetworks();
              if (wifiCount == 0) {
                u8g2.userInterfaceMessage("No Networks", "Found!", "", " Ok ");
                wsSel = 0;
              }
            } else if (wsSel > 1) {
              String wifiPass = keyboard("");
              WiFi.begin(WiFi.SSID(wsSel - 2), wifiPass);
              wifiConnectScreen();
              wsSel = 0;
            }
          }
        }
      }
    }
    if (subSel == 4) {
      if (!wStatus) {
        int wsSel = 1;
        String wifiSSID = "";
        String wifiPass = "";
        while (wsSel != 0) {
          String wConItems =
          "SSID: " + (wifiSSID == "" ? "<Required!>" : wifiSSID) + "\n"
        + "Pass: " + (wifiPass == "" ? "<None>" : wifiPass) + "\n"
        + (wifiSSID == "" ? "[type SSID!]" : "Connect!");
          wsSel = u8g2.userInterfaceSelectionList("Manual connect", wsSel, wConItems.c_str());
          if (wsSel == 1) wifiSSID = keyboard(wifiSSID);
          if (wsSel == 2) wifiPass = keyboard(wifiPass);
          if (wsSel == 3 && wifiSSID != "") {
            WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
            wifiConnectScreen();
            wsSel = 0;
          }
        }
      }
    }
    if (subSel == 6) {
      WiFi.disconnect();
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Disconnecting...") / 2, 32, "Disconnecting...");
      u8g2.sendBuffer();
      delay(500);
    }
  }
}

void connectMenu() {
  const char items[] =
    "USB HID\n"
    "Bluetooth\n"
    "WiFi Settings\n"
    "Keyboard Layout";
  int sel = 1;
  while (sel != 0) {
    sel = u8g2.userInterfaceSelectionList("Connection", sel, items);
    if (sel == 1) {
      int subSel = 1;
      while (subSel != 0) {
        String usbItems =
          "Status: " + String(tud_ready() ? "[OK]" : "[Offline]") + "\n"
          + "Always Report: " + String(alwaysReport ? "[On]" : "[Off]") + "\n"
          + "Polling Rate: " + String(1000000 / LOOP_INTERVAL_US) + " Hz";
        //  + "Layout: " + layoutName[layoutType]; // moved because this can also be used on ble hid
        subSel = u8g2.userInterfaceSelectionList("USB HID", subSel, usbItems.c_str());
        if (subSel == 1) {
          u8g2.clearBuffer();
          u8g2.drawStr(64 - u8g2.getStrWidth("Refreshing...") / 2, 32, "Refreshing...");
          u8g2.sendBuffer();
          delay(200);
        }
        if (subSel == 2) alwaysReport = !alwaysReport;
        else if (subSel == 3) {
          const char pollingList[] =
            "125 Hz\n"
            "250 Hz\n"
            "500 Hz\n"
            "1000 Hz\n"
            "2000 Hz\n"
            "4000 Hz [!]\n"
            "8000 Hz [!]";
          int pollSel = u8g2.userInterfaceSelectionList("Polling Rate", 1, pollingList);
          switch (pollSel) {
            case 1: LOOP_INTERVAL_US = 8000; break;
            case 2: LOOP_INTERVAL_US = 4000; break;
            case 3: LOOP_INTERVAL_US = 2000; break;
            case 4: LOOP_INTERVAL_US = 1000; break;
            case 5: LOOP_INTERVAL_US = 500; break;
            case 6: {
              int confirm = u8g2.userInterfaceMessage(
                "Warning!",
                "4000Hz may unstable",
                "Continue?",
                " Yes \n No "
              );
              if (confirm == 1) LOOP_INTERVAL_US = 250;
              break;
            }
            case 7: {
              int confirm = u8g2.userInterfaceMessage(
                "Warning!",
                "8000Hz is unstable",
                "Continue?",
                " Yes \n No "
              );
              if (confirm == 1) LOOP_INTERVAL_US = 125;
              break;
            }
            default: break;
          }
        }
      }
    }
    if (sel == 2) {
      int subSel = 1;
      subSel = u8g2.userInterfaceMessage(
        "Attention [1 / 2]",
        "BLE function is not",
        "stable",
        " Next>> "
      );
      if (subSel == 0) continue;
      subSel = u8g2.userInterfaceMessage(
        "Attention [2 / 2]",
        "Make sure to calli-",
        "brate before use",
        " Ok "
      );
      if (subSel == 0) continue;
      int BLEfunction = -1;
      while (subSel != 0) {
        String bleItems =
          "Device Name: " + btName + "\n"
          + "--------- Mode ---------\n"
          + "Keyboard\n"
          + "Air Mouse\n"
          + "Gamepad";
        subSel = u8g2.userInterfaceSelectionList("Bluetooth", subSel, bleItems.c_str());
        if (subSel == 1) {
          btName = keyboard(btName);
        } else {
          BLEfunction = subSel - 3;
          subSel = 0;
        }
      }
      if (BLEfunction == 0) mwk();
      else if (BLEfunction == 1) marm();
      else if (BLEfunction == 2) mgp();
    }
    if (sel == 3) {
      wifiMenu();
    }
    else if (sel == 4) {
      int layChange = 1;
      while (layChange != 0) {
        String layoutName[] = {
          "Standart",
          "WASD",
          "Arrows",
          "Shortcuts",
          "For BLE Key",
          "Custom"
        };
        String tmpName = "";
        for (int i = 0; i < 6; i++) {
          tmpName += (i == layoutType ? "> " : "") + layoutName[i] + (i == layoutType ? " <" : "");
          if (i < 5) tmpName += "\n";
        }
        layChange = u8g2.userInterfaceSelectionList("Layout", layoutType + 1, tmpName.c_str());
        if (layChange == 0) continue;
        layoutType = layChange != 0 ? layChange - 1 : layoutType;
        if (layoutType == 5) {
          int layoutSel = 1;
          while (layoutSel != 0) {
            String nowLayout = "";
            for (int i = 0; i < 6; i++) {
              if (i < 3) nowLayout += "Sw" + String(i + 1) + ": ";
              else nowLayout += "F" + String(i - 2) + ": ";
              nowLayout += codeToName(layout[i]);
              if (i < 5) nowLayout += "\n";
            }
            layoutSel = u8g2.userInterfaceSelectionList("Custom Layout", layoutSel, nowLayout.c_str());
            if (layoutSel > 0) {
              int ckeycode = u8g2.userInterfaceSelectionList("Change Key", 1, buttonName);
              layout[layoutSel - 1] = buttonCode[ckeycode - 1];
            }
          }
        } else {
          layChange = 0;
          for (int i = 0; i < 6; i++) {
            layout[i] = preLayout[layoutType][i];
          }
        }
      }
    }
  }
}

void displaySetting() {
  int subSel = 1;
  const char lazyAss[] =
    "1s\n"
    "5s\n"
    "10s\n"
    "20s\n"
    "30s\n"
    "1m\n"
    "5m\n"
    "10m"
  ;
  while (subSel != 0) {
    const String kaoOrSomethingIdk[] = {
      "Mufuki",
      "shy smile",
      "happy wide smile",
      "shock surprise",
      "angry frown",
      "crying shy",
      "shrug",
      "annoyed",
      "bashful",
      "excited grin",
      "lenny face",
      "Custom"
    };
    String menuItem =
    "Brightness: " + String(screenBri) + "\n"
    + "Screen on: " + String(screenSaveDuration / 1000) + "s\n"
    + "Screen save: " + String((screenOffDuration - screenSaveDuration) / 1000) + "s\n";
    if (logoType == 0 || logoType == 12)
      menuItem += "Icon: \"" + screenLogo + "\"";
    else
      menuItem += "Icon: \"" + String(kaoOrSomethingIdk[logoType - 1]) + "\"";
    subSel = u8g2.userInterfaceSelectionList("Display", subSel, menuItem.c_str());
    if (subSel == 1) {u8g2.userInterfaceInputValue("Brightness (0 - 255)", " ", &screenBri, 0, 255, 3, " "); u8g2.setContrast(screenBri);}
    if (subSel == 2) {
      switch (u8g2.userInterfaceSelectionList("Screen on", 1, lazyAss)) {
        case 1: screenSaveDuration = 1000; screenOffDuration += 1000; break;
        case 2: screenSaveDuration = 5000; break;
        case 3: screenSaveDuration = 10000; break;
        case 4: screenSaveDuration = 20000; break;
        case 5: screenSaveDuration = 30000; break;
        case 6: screenSaveDuration = 60000; break;
        case 7: screenSaveDuration = 300000; break;
        case 8: screenSaveDuration = 600000; break;
        default: break;
      }
    }
    if (subSel == 3) {
      switch (u8g2.userInterfaceSelectionList("Screen on", 1, lazyAss)) {
        case 1: screenOffDuration = 1000; break;
        case 2: screenOffDuration = 5000; break;
        case 3: screenOffDuration = 10000; break;
        case 4: screenOffDuration = 20000; break;
        case 5: screenOffDuration = 30000; break;
        case 6: screenOffDuration = 60000; break;
        case 7: screenOffDuration = 300000; break;
        case 8: screenOffDuration = 600000; break;
        default: break;
      }
    }
    if (subSel == 4) {
      String tmp = "";
      for (int i = 0; i < 12; i++) {
        tmp += kaoOrSomethingIdk[i];
        if (i < 11) tmp += "\n";
      }
      int logSel = u8g2.userInterfaceSelectionList("Display Icon", 1, tmp.c_str());
      if (logSel == 1) {
        screenLogo = "Mufuki";
        logoType = 0;
      } else if (logSel > 1 && logSel < 12) logoType = logSel;
      else if (logSel != 0) {
        screenLogo = keyboard(screenLogo);
        logoType = 12;
      }
    }
  }
}

void mpuMenu() {
  const char useless[] =
    "Calibrate\n"
    "Inclinometer\n"
    "Speedometer"
  ;
  int sel = 1;
  while (sel != 0) {
    sel = u8g2.userInterfaceSelectionList("MPU", sel, useless);
    if (sel == 1) {
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Calibrating") / 2, 32, "Calibrating...");
      u8g2.sendBuffer();
      mpu.calcOffsets();
      delay(1000);
    }
    if (sel == 2) {
      bool running = true;
      while (running) {
        mpu.update();
        int dx = mpu.getAngleY();
        int dy = mpu.getAngleX();
        int px = map(constrain(dx, -70, 70), -70, 70, 32, 96);
        int py = map(constrain(dy, -70, 70), -70, 70, 0, 64);
        u8g2.clearBuffer();
        u8g2.drawLine(64, 0, 64, 64);
        u8g2.drawLine(0, 32, 128,  32);
        u8g2.drawCircle(px, py, 4);
        u8g2.sendBuffer();
        if (getButton() == 3) running = false;
      }
    }
    if (sel == 3) {
      float velX = 0, velY = 0, velZ = 0;
      float ax_f = 0, ay_f = 0, az_f = 0;
      const float ALPHA = 0.4;
      unsigned long lastTime = micros();
      unsigned long ldu = millis();
      bool kph = false;
      bool running = true;
      while (running) {
        mpu.update();

        unsigned long now = micros();
        float dt = (now - lastTime) / 1000000.0f; // s
        if (dt <= 0) return;
        lastTime = now;

        // raw accel in g
        float ax = mpu.getAccX();
        float ay = mpu.getAccY();
        float az = mpu.getAccZ();

        // simple low-pass to reduce high-frequency noise (helps angle calc)
        ax_f = ALPHA * ax + (1.0 - ALPHA) * ax_f;
        ay_f = ALPHA * ay + (1.0 - ALPHA) * ay_f;
        az_f = ALPHA * az + (1.0 - ALPHA) * az_f;

        // compute roll & pitch from accel (radians)
        float roll  = atan2(ay_f, az_f);
        float pitch = atan2(-ax_f, sqrt(ay_f * ay_f + az_f * az_f));

        const float G = 9.80665f;
        // gravity components in m/s^2 (body frame)
        float gX = -sin(pitch) * G;
        float gY =  sin(roll)  * cos(pitch) * G;
        float gZ =  cos(roll)  * cos(pitch) * G;

        // measured accel in m/s^2
        float accX = ax * G;
        float accY = ay * G;
        float accZ = az * G;

        // linear acceleration (remove gravity)
        float linAccX = accX - gX;
        float linAccY = accY - gY;
        float linAccZ = accZ - gZ;

        // small deadband to ignore sensor noise
        const float DEAD = 0.05f;
        if (fabs(linAccX) < DEAD) linAccX = 0;
        if (fabs(linAccY) < DEAD) linAccY = 0;
        if (fabs(linAccZ) < DEAD) linAccZ = 0;

        // integrate -> velocity
        velX += linAccX * dt;
        velY += linAccY * dt;
        velZ += linAccZ * dt;

        // time-independent damping (exponential) to avoid dependence on sample rate
        // choose tau (s) = 10s -> factor = exp(-dt/tau)
        const float tau = 10.0f;
        float damp = expf(-dt / tau);
        velX *= damp;
        velY *= damp;
        velZ *= damp;

        float speed = sqrtf(velX*velX + velY*velY + velZ*velZ);

        if (millis() - ldu > 200) {
          u8g2.clearBuffer();
          String tmp = String(int(speed * (kph ? 3.6 : 1))) + (kph ? "km/h" : "m/s");
          u8g2.setFont(u8g2_font_fub20_tf);
          u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 40, tmp.c_str());
          u8g2.setFont(u8g2_font_gulim11_t_korean1);
          tmp = "X: " + String(int(velX)) + "m/s | Y: " + String(int(velY)) + "m/s | Z: " + String(int(velZ)) + "m/s";
          u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 54, tmp.c_str());
          u8g2.sendBuffer();
          ldu = millis();
        }
        int btn = getButton();
        if (btn == 0) kph = !kph;
        if (btn == 1) {
          velX = 0.00;
          velY = 0.00;
          velZ = 0.00;
        }
        if (btn == 3) running = false;
        delay(12);
      }
    }
  }
}

void showDebug() {
  bool running = true;
  int page = 0;
  const int pages = 4;
  u8g2.setFont(u8g2_font_5x8_mf);
  while (running) {
    u8g2.clearBuffer();
    updateInput();
    int btn = getButton();
    if (btn == 0) page = (page + (pages - 1)) % pages;
    if (btn == 1) page = (page + 1) % pages;
    if (btn == 3) running = false;
    switch (page) {
      case 0: {
        for (int i = 0; i < 3; i++) {
          String tmp = "S" + String(i + 1) + ":" + String(hallVal[i]);
          u8g2.drawStr(0, 10 + i * 10, tmp.c_str());
          tmp = "Raw" + String(i + 1) + ":" + String(rawVal[i]);
          u8g2.drawStr(78, 10 + i * 10, tmp.c_str());
          tmp = "Min" + String(i + 1) + ":" + String(int(calMin[i]));
          u8g2.drawStr(0, 40 + i * 10, tmp.c_str());
          tmp = "Max" + String(i + 1) + ":" + String(int(calMax[i]));
          u8g2.drawStr(78, 40 + i * 10, tmp.c_str());
        }
        break;
      }
      case 1: {
        String tmp = "chip:" + String(ESP.getChipModel());
        u8g2.drawStr(0, 10, tmp.c_str());
        tmp = "cores:" + String(ESP.getChipCores());
        u8g2.drawStr(0, 20, tmp.c_str());
        tmp = "free:" + String(ESP.getFreeHeap()) + "b";
        u8g2.drawStr(0, 30, tmp.c_str());
        tmp = "heap:" + String(ESP.getHeapSize()) + "b";
        u8g2.drawStr(0, 40, tmp.c_str());
        tmp = "flash:" + String(ESP.getFlashChipSize()) + "b";
        u8g2.drawStr(0, 50, tmp.c_str());
        tmp = "temp:" + String(temperatureRead()) + " deg";
        u8g2.drawStr(0, 60, tmp.c_str());
        break;
      }
      case 2: {
        String tmp = "ip:" + WiFi.localIP().toString();
        u8g2.drawStr(0, 10, tmp.c_str());
        tmp = "ssid:" + String(WiFi.SSID());
        u8g2.drawStr(0, 20, tmp.c_str());
        tmp = "rssi:" + String(WiFi.RSSI());
        u8g2.drawStr(0, 30, tmp.c_str());
        tmp = "mac:" + String(WiFi.macAddress());
        u8g2.drawStr(0, 40, tmp.c_str());
        tmp = "adcDeadZone:" + String(deadZone);
        u8g2.drawStr(0, 50, tmp.c_str());
        break;
      }
      case 3: {
        mpu.update();
        u8g2.drawStr(0, 10, "[XYZ]");
        u8g2.drawStr(0, 20, "acc:"); u8g2.drawStr(30, 20, String(mpu.getAccX()).c_str()); u8g2.drawStr(70, 20, String(mpu.getAccY()).c_str()); u8g2.drawStr(110, 20, String(mpu.getAccZ()).c_str());
        u8g2.drawStr(0, 30, "ang:"); u8g2.drawStr(30, 30, String(mpu.getAngleX()).c_str()); u8g2.drawStr(70, 30, String(mpu.getAngleY()).c_str()); u8g2.drawStr(110, 30, String(mpu.getAngleZ()).c_str());
        u8g2.drawStr(0, 40, "gyr:"); u8g2.drawStr(30, 40, String(mpu.getGyroX()).c_str()); u8g2.drawStr(70, 40, String(mpu.getGyroY()).c_str()); u8g2.drawStr(110, 40, String(mpu.getGyroZ()).c_str());
        u8g2.drawStr(0, 50, "tmp:"); u8g2.drawStr(30, 50, String(mpu.getTemp()).c_str());
        break;
      }
      default: break;
    }
    u8g2.sendBuffer();
  }
}

void deadCalib() {
  const char menuItems[] = 
    "Auto Calibrate\n"
    "Manual Set"
  ;
  int option = 1;
  while (option != 0) {
    option = u8g2.userInterfaceSelectionList("DeadZone Calibrate", option, menuItems);
    if (option == 1) {
      for (int i = 0; i < 5; i++){
        u8g2.clearBuffer();
        u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
        u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
        String tmp = "[" + String(5 - i) + "s]";
        u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
        u8g2.sendBuffer();
        delay(1000);
      }
      u8g2.clearBuffer();
      u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
      u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
      String tmp = "[Started]";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
      u8g2.sendBuffer();
      int aMax = 0;
      int aMin = 4095;
      delay(1000);
      for (int sw = 0; sw < 3; sw++) {
        u8g2.clearBuffer();
        u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
        u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
        String tmp = "[" + String(sw + 1) + "/3]";
        u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
        u8g2.sendBuffer();
        for (int i = 0; i < 2000; i++) {
          int adcVal = 0;
          if (doFilter) {
            if (filterType == 0) {
              adcVal = overSample(sw);
            } else {
              adcVal = expoMovAvr(sw);
            }
          } else adcVal = analogRead(adcPins[sw]);
          if (adcVal > aMax) aMax = adcVal;
          if (adcVal < aMin) aMin = adcVal;
          delay(1);
        }
      }
      deadZone = aMax - aMin;
    }
    else if (option == 2) {
    const int deadZoneVals[] = {0, 16, 32, 64, 128, 256};
    const char idk[] = "0\n16\n32\n64\n128\n256";
    int sel = u8g2.userInterfaceSelectionList("Manual Set", 0, idk);
    int deadZone = deadZoneVals[sel];
    }
  }
  u8g2.userInterfaceMessage(
    "Remember to",
    "go to calibration",
    "menu to recalibrate",
    " Ok "
  );
}

void otherMenu() {
  int sel = 1;
  const char menuItems[] =
    "Display\n"
    "Deadzone Calirate\n"
    "MPU\n"
    "Web App\n"
    "Debug"
  ;
  while (sel != 0) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("Other", sel, menuItems);
    switch (sel) {
      case 1: displaySetting(); break;
      case 3: mpuMenu(); break;
      case 5: showDebug(); break;
      case 2: deadCalib(); break;
      default: break;
    }
  }
}

void about() {
  l.setBrightness(255);
  l.fill(l.Color(255, 0, 255));
  l.show();
  bool running = true;
  for (int i = 0; i < 10; i++) {
    u8g2.clearBuffer();
    u8g2.drawXBMP(20, 20, 88, 232, logoKao[i]);
    u8g2.setDrawColor(0);
    u8g2.drawBox(20, 0, 88, 20);
    u8g2.drawBox(20, 40, 88, 24);
    u8g2.setDrawColor(1);
    u8g2.sendBuffer();
    delay(500);
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub20_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
  u8g2.sendBuffer();
  delay(1000);
  for (int i = 0; i < 23; i++) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth("v1.8.2"))/2, 76 - i, "v1.8.2");
    u8g2.sendBuffer();
    delay(20);
  }
  uint16_t h = 0;
  while (running) {
    updateInput();
    for (int i = 0; i < 3; i++) {
      ledOutput[i] = 0;
      if (applyEffect[i]) {
        singleFade[i].active = true;
        singleFade[i].startTime = millis();
        applyEffect[i] = false;
      }
    }
    updateSingleFade();
    for (int i = 0; i < 3; i++) ledcWrite(i, ledOutput[i]);
    h += 256;
    l.rainbow(h, 1, 255, rgbBri, true);
    l.show();
    if (getButton() == 3) running = false;
    delay(60);
  }
  l.setBrightness(0);
  l.show();
}

void mainMenu() {
  const char menu_items[] =
    "Calibration\n"
    "Input handling\n"
    "Filter\n"
    "Effects\n"
    "Connection\n"
    "Other\n"
    "OTA Update\n"
    "About";

  for (int i = 0; i < 3; i++) ledcWrite(i, 0);
  l.setBrightness(0);
  l.show();
  u8g2.clearBuffer();
  u8g2.setPowerSave(0);
  u8g2.setFont(u8g2_font_fub20_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr((128 - u8g2.getStrWidth("Main Menu"))/2, 54, "Main Menu");
  u8g2.sendBuffer();
  while (!digitalRead(btnPins[3])) delay(10);
  int sel = 1;
  while (sel > 0) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("Main Menu", sel, menu_items);
    switch (sel) {
      case 1: calibMenu(); break;
      case 2: inputMenu(); break;
      case 3: filtMenu(); break;
      case 4: effectMenu(); break;
      case 5: connectMenu(); break;
      case 6: otherMenu(); break;
      case 7:
        if (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) {;
          while (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) {
            int c = u8g2.userInterfaceMessage(
            "No Connection",
            "Please connect",
            "to WiFi first",
            " WiFi \n Back "
            );
            if (c == 1) wifiMenu();
            else break;
          }
          if (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) break;
        }
        otaUpdate();
        break;
      case 8: about(); break;
      default:
        break;
    }
  }
  for (int i = 0; i < 3; i++) {
    ledcWrite(i, 0);
    applyEffect[i] = 0;
  }
  if (rgb) {
    l.setBrightness(rgbBri);
    l.fill(l.Color(color[0], color[1], color[2]));
    l.show();
  } else {
    l.setBrightness(0);
    l.show();
  }
}

void setup() {

  // Wireless
  BLEDevice::deinit(true);
  WiFi.mode(WIFI_OFF);

  // Hardware setup
  l.begin();
  l.fill(l.Color(255, 0, 0));
  l.show();
  Wire.begin(8, 9);
  u8g2.begin(btnPins[1], btnPins[2], btnPins[0], btnPins[0], btnPins[2], btnPins[3]);
  u8g2.setContrast(screenBri);
  u8g2.setFontMode(1);
  u8g2.enableUTF8Print();
  u8g2.setFontRefHeightAll();
  l.fill(l.Color(255, 255, 0));
  l.show();
  byte status = mpu.begin();
  l.fill(status == 0 ? l.Color(0, 255, 255) : l.Color(255, 0, 0));
  l.show();
  if (status != 0) delay(1000);
  analogSetAttenuation(ADC_11db);

  // PinMode
  for (int i = 0; i < 3; i++) pinMode(adcPins[i], INPUT);
  for (int i = 0; i < 3; i++) pinMode(ledPins[i], OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(btnPins[i], INPUT_PULLUP);

  // LEDs
  for (int i = 0; i < 3; i++) {
    ledcSetup(i, 5000, 8);
    ledcAttachPin(ledPins[i], i);
    ledcWrite(i, 0);
  }

  l.fill(l.Color(0, 255, 0));
  l.setBrightness(0);
  l.show();
  waitIDLE = millis();
  if (digitalRead(btnPins[3])) dev.begin();
}

void loop() {

  if (micros() - lastLoopTime < LOOP_INTERVAL_US) return;
  lastLoopTime += LOOP_INTERVAL_US;

  // F4 Key (Menu)
  if (digitalRead(btnPins[3]) == LOW) {
    if (!bt4Hold) {
      bt4Hold = true;
      bt4time = millis();
    }
    if (millis() - bt4time > 500 && bt4Hold) {mainMenu(); waitIDLE = millis(); fromMenu = true;}
    if (screenOff) u8g2.setPowerSave(0);
    screenWait = false;
    screenOff = false;
  } else {
    if (bt4Hold && !fromMenu) {
      if (millis() - waitIDLE < 5000) waitIDLE = millis() - 5000;
      else {
        waitIDLE = millis();
        u8g2.setPowerSave(0);
        screenWait = false;
        screenOff = false;
      }
    }
    fromMenu = false;
    bt4Hold = false;
  }

  // Input Handling
  switch (inputHandler) {
    case 0: inputTypeDigitalEmulation(); break;
    case 1: inputTypeHysteresisHandling(); break;
    case 2: inputTypeDynamicActuation(); break;
    default: inputTypeDigitalEmulation(); break;
  }
  if (alwaysReport) needReport = true;

  // USB Report
  if (tud_ready() || alwaysReport) {
    if (needReport) {
      uint8_t keycodes[6] = {0};
      uint8_t idx = 0;
      for (int i = 0; i < 6; i++)
        if (nowPress[i]) keycodes[idx++] = layout[i];
      tud_hid_keyboard_report(dev.report_id, 0, keycodes);
    }
  }

  // Effects
  if (underGlow) {
    switch (glowType) {
      case 0:
        for (int i = 0; i < 3; i++) {
          ledOutput[i] = 0;
          if (applyEffect[i]) {
            singleFade[i].active = true;
            singleFade[i].startTime = millis();
            applyEffect[i] = false;
          }
        }
        updateSingleFade();
        for (int i = 0; i < 3; i++) ledcWrite(i, ledOutput[i]);
      break;
      case 1:
        for (int i = 0; i < 3; i++) {
          ledOutput[i] = 0;
          if (applyEffect[i]) {
            addRippleWave(i);
            applyEffect[i] = false;
          }
        }
        updateRipple();
        for (int i = 0; i < 3; i++) ledcWrite(i, ledOutput[i]);
      break;
      case 2:
        udgSmooth(); // handled in its own function
      break;
      case 3:
        udgBurnIn(); // also this
      break;
      case 4:
        udgAnalog();
      break;
      case 5:
        udgSoild();
      break;
      default:
      break;
    }
  }

  if (rgb && doRainbow) {
    if (millis() - lastRGBUpdate > rgbInterval * 10) {
      lastRGBUpdate = millis();
      static uint16_t hue = 0;
      hue += 128 + rainbowStep;
      l.rainbow(hue, 1, 255, rgbBri, true);
      l.show();
    }
  }

  // Screen
  if (millis() - waitIDLE < screenSaveDuration) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_mf);
    IPAddress ip = WiFi.localIP();
    u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
    if (WiFi.status() != WL_CONNECTED)
      u8g2.drawStr(0, 18, "WiFi: [Off]");
    else u8g2.drawStr(0, 18, ("Device ip: " + ip.toString()).c_str());
    switch (inputHandler) {
      case 0:
        u8g2.drawStr(0, 26, "Input: Digital Emulation");
        u8g2.drawStr(0, 34, ("Actuation: " + String(actuation)).c_str());
        break;
      case 1:
        u8g2.drawStr(0, 26, "Input: Hysteresis Handling");
        u8g2.drawStr(0, 34, ("Upper: " + String(upperThreshold)).c_str());
        u8g2.drawStr(0, 42, ("Lower: " + String(lowerThreshold)).c_str());
        break;
      case 2:
        u8g2.drawStr(0, 26, "Input: Dynamic Actuation");
        u8g2.drawStr(0, 34, ("Window Size: " + String(windowSize)).c_str());
        // The foot value is dynamic, so not display it
        break;
      default: u8g2.drawStr(0, 26, "Input: Digital Emulation"); break;
    }
    //float maxPress = 0.00;
    for (int i = 0; i < 3; i++) {
      if (hallVal[i] > 0.05) waitIDLE = millis();
      u8g2.drawFrame(43 * i - 1, 55, 44, 10);
      u8g2.drawBox(43 * i, 55, (int)(hallVal[i] * 43), 10);
      u8g2.setDrawColor(2);
      String line = String(hallVal[i], 2);
      u8g2.drawStr(1 + 43 * i, 63, line.c_str());
      u8g2.setDrawColor(1);
      //maxPress = max(maxPress, hallVal[i]);
    }
    u8g2.sendBuffer();
  }
  else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
    if (!screenWait) {
      screenSaver("NoID");
      screenWait = true;
    }
  }
  else {
    if (!screenOff) {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      u8g2.setPowerSave(1);
      screenOff = true;
    }
  }
}
