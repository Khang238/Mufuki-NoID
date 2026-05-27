#pragma once
#include "global.h"
#include "mapping.h"
#include "input.h"

#define MODE_KEYBOARD 0
#define MODE_GAMEPAD  1
#define MODE_MOUSE    2

Profile createGamepadTestProfile();

extern Profile testProfile;
extern OutputState testOutputState;
extern Profile prf;
extern HIDgamepad devgp;

void setupSandbox1();
void loopSandbox1();

void setupSandbox2();
void loopSandbox2();