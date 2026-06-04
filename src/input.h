#pragma once
#include "global.h"
#include "profile.h"

extern float y[3];

int overSample(int chan, int samples = 16);

int expoMovAvr(int chan, float alpha = 0.05);

void readHall(int i);

void inputTypeDigitalEmulation();

void inputTypeHysteresisHandling();

void inputTypeDynamicActuation();

void updateInput();

struct MorseNode {
  char letter;
  MorseNode *dot;
  MorseNode *dash;
};

void drawWrappedText(U8G2 &u8g2, int x, int y, int maxWidth, const char *text);

int getButton();

extern MorseNode root;

void addMorse(const char *code, char letter);

extern bool unsignedCharacter;
char decodeMorse(const char *code);

extern bool genMorse;
void setupMorse();

extern bool morseMode;

String keyboard(String text);

void keypadMUI();

void mouseMUI();

void gamepadMUI();

void handleKeypad();

void handleMouse();

void handleGamepad();