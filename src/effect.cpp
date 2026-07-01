#include "effect.h"
#include "profile.h"

Wave waves[MAX_WAVES];
LedFade singleFade[3];

int waveIdx = 0;
float smtLed[] = {0.00, 0.00, 0.00};
float burnLevel[] = {0.00, 0.00, 0.00};
unsigned long lastPressT[] = {0, 0, 0};
int decayTime = 5000;
int spamReq = 125; // ~8 CPS, if slower then ignore the click and not burn

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

void underGlowUpdate() {
  if (!prf.underGlow) return;
  if (analogLed) for (int i = 0; i < 3; i++) ledcWrite(i, ledOutput[i]);
  else {
    for (int i = 0; i < 3; i++) b.setPixelColor(i, b.Color((prf.color[0] * ledOutput[i] * 255) / (255 * maxBri), (prf.color[1] * ledOutput[i] * 255) / (255 * maxBri), (prf.color[2] * ledOutput[i] * 255) / (255 * maxBri)));
    b.show();
  }
}

void setUnderGlowPixel(int idx, uint8_t red, uint8_t green, uint8_t blue, uint8_t bri) {
  if (analogLed) {
    if (idx < 3) ledOutput[idx] = (prf.color[idx] * bri * prf.rgbBri) / (255 * maxBri);
  } else {
    if (idx < 3) b.setPixelColor(idx, b.Color((prf.color[0] * bri * prf.rgbBri) / (255 * maxBri), (prf.color[1] * bri * prf.rgbBri) / (255 * maxBri), (prf.color[2] * bri * prf.rgbBri) / (255 * maxBri)));
    b.show();
  }
}

void udgSmooth() {
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();
  for (int i = 0; i < 3; i++) {
    smtLed[i] += ((nowPress[i] ? 1.00 : 0.00) - smtLed[i]) * 0.32;
    ledOutput[i] = (int)(smtLed[i] * maxBri);
  }
  underGlowUpdate();
}

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
    ledOutput[i] = (int)(burnLevel[i] * 255 > smtLed[i] * maxBri ? burnLevel[i] * 255 : smtLed[i] * maxBri); // BURN YOUR EYES
  }
  underGlowUpdate();
}

void udgAnalog() {
  for (int i = 0; i < 3; i++) {
    ledOutput[i] = (int)(hallVal[i] * maxBri);
  }
  underGlowUpdate();
}

void udgSoild() {
  for (int i = 0; i < 3; i++) {
    ledOutput[i] = nowPress[i] ? maxBri : 0;
  }
  underGlowUpdate();
}