#include "global.h"

int overSample(int chan, int samples = 16) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(adcPins[chan]);
  }
  return sum / samples;
}

float y[] = {0.0, 0.0, 0.0};
int expoMovAvr(int chan, float alpha = 0.05) {
  float x = analogRead(adcPins[chan]);
  y[chan] = y[chan] + alpha * (x - y[chan]);
  return y[chan];
}

void readHall(int i) {
  if (doFilter) {
    if (filterType == 0) rawVal[i] = overSample(i);
    else rawVal[i] = expoMovAvr(i);
  } else rawVal[i] = analogRead(adcPins[i]);
  hallVal[i] = constrain(
    (float)(rawVal[i] - calMin[i] - deadZone) /
    (float)(calMax[i] - calMin[i] - 2 * deadZone),
    0.00, 1.00
  );
}

void inputTypeDigitalEmulation() {
  for (int i = 0; i < 6; i++) {
    if (i < 3) {
      readHall(i);
      nowPress[i] = (hallVal[i] > actuation);
      if ((nowPress[i] != lastPress[i]) && nowPress[i]) {
        applyEffect[i] = true;
      }
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
      readHall(i);
      if (hallVal[i] > upperThreshold)
        nowPress[i] = true;
      else if (hallVal[i] < lowerThreshold)
        nowPress[i] = false;
      if ((nowPress[i] != lastPress[i]) && nowPress[i]) {
        applyEffect[i] = true;
      }
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
      readHall(i);
      if (hallVal[i] > windowFoot[i] + windowSize) {
        nowPress[i] = true;
        windowFoot[i] = hallVal[i] - windowSize;
      }
      else if (hallVal[i] < windowFoot[i]) {
        nowPress[i] = false;
        windowFoot[i] = hallVal[i];
      }
      if (hallVal[i] == 0.0) windowFoot[i] = 0.0;
      if (hallVal[i] == 1.0) windowFoot[i] = 1.0 - windowSize;
      if ((nowPress[i] != lastPress[i]) && nowPress[i]) {
        applyEffect[i] = true;
      }
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