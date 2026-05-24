#pragma once
#include "global.h"

struct LedFade {
  bool active;
  uint32_t startTime;
};

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

constexpr int MAX_WAVES      = 5;   // those
constexpr int WAVE_DURATION  = 300; // will
constexpr int WAVE_DELAY     = 80;  // be
constexpr int DELIGHT        = 50;  // configurable (in more 10 years i guess)

extern LedFade singleFade[3];
extern Wave waves[MAX_WAVES];

extern int waveIdx;
extern float smtLed[3];
extern float burnLevel[3];
extern unsigned long lastPressT[3];
extern int decayTime;
extern int spamReq;

void updateSingleFade();

void addRippleWave(int center);

void updateRipple();

void underGlowUpdate();

void setUnderGlowPixel(int idx, uint8_t red, uint8_t green, uint8_t blue, uint8_t bri);

void udgSmooth();

void udgBurnIn();

void udgAnalog();

void udgSoild();