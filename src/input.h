#pragma once
#include "global.h"

extern float y[3];

int overSample(int chan, int samples = 16);

int expoMovAvr(int chan, float alpha = 0.05);

void readHall(int i);

void inputTypeDigitalEmulation();

void inputTypeHysteresisHandling();

void inputTypeDynamicActuation();

void updateInput();