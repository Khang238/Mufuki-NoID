#pragma once
#include "global.h"
#include "tusb.h"

#define MACRO_MAX_ACTIONS   24
#define MACRO_MAX_HELD_KEYS 6
#define MACRO_TEXT_MAX_LEN  32

#define MACRO_TAP_HOLD_MS 10

#define ANTICHEAT_HISTORY_SIZE     8
#define ANTICHEAT_MIN_SAMPLES      5
#define ANTICHEAT_MIN_INTERVAL_MS  45UL
#define ANTICHEAT_MAX_JITTER_MS    2UL
#define ANTICHEAT_LOCKOUT_MS       30000UL

#define MACRO_MOD_NONE  0x00
#define MACRO_MOD_CTRL  KEYBOARD_MODIFIER_LEFTCTRL
#define MACRO_MOD_SHIFT KEYBOARD_MODIFIER_LEFTSHIFT
#define MACRO_MOD_ALT   KEYBOARD_MODIFIER_LEFTALT
#define MACRO_MOD_GUI   KEYBOARD_MODIFIER_LEFTGUI
#define MACRO_MOD_RCTRL  KEYBOARD_MODIFIER_RIGHTCTRL
#define MACRO_MOD_RSHIFT KEYBOARD_MODIFIER_RIGHTSHIFT
#define MACRO_MOD_RALT   KEYBOARD_MODIFIER_RIGHTALT
#define MACRO_MOD_RGUI   KEYBOARD_MODIFIER_RIGHTGUI

enum macType : uint8_t {
  MACRO_PRESS,
  MACRO_HOLD,
  MACRO_RELEASE,
  MACRO_TEXT,
  MACRO_DELAY
};

struct macroAct {
  uint8_t keycode;
  uint8_t modifier;
  char text[MACRO_TEXT_MAX_LEN];
  macType mType;
  unsigned long actDelay;
};

struct Macro {
  int macCount = 0;
  macroAct actions[MACRO_MAX_ACTIONS];
};

Macro macQuick[3];

bool addAct(Macro &m, uint8_t keycode, macType mt = MACRO_PRESS, unsigned long adl = 500, uint8_t modifier = MACRO_MOD_NONE);
bool insertAct(Macro &m, int index, uint8_t keycode, macType mt = MACRO_PRESS, unsigned long adl = 500, uint8_t modifier = MACRO_MOD_NONE);
bool editAct(Macro &m, int index, uint8_t keycode, macType mt, unsigned long adl, uint8_t modifier = MACRO_MOD_NONE);
bool removeAct(Macro &m, int index);
bool clearMacro(Macro &m);

bool addTextAct(Macro &m, const char *text, unsigned long adl = 8);
bool insertTextAct(Macro &m, int index, const char *text, unsigned long adl = 8);
bool editTextAct(Macro &m, int index, const char *text, unsigned long adl = 8);

bool addDelayAct(Macro &m, unsigned long adl);

bool macroHasUnbalancedHolds(const Macro &m);

bool saveMacro(const char *path, Macro &m);
bool loadMacro(const char *path, Macro &m);

bool executeMacro(Macro &m, uint8_t triggerId = 0);

void macroReleaseAll();

bool macroIsRunning();

bool macroIsLocked();
unsigned long macroLockoutRemainingMs();