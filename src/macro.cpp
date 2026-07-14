#include "macro.h"

static uint8_t heldKeycodes[MACRO_MAX_HELD_KEYS] = {0};
static uint8_t heldCount = 0;
static uint8_t heldModifier = 0;
static bool macroRunning = false;

static unsigned long acTimestamps[ANTICHEAT_HISTORY_SIZE] = {0};
static uint8_t acTriggerIds[ANTICHEAT_HISTORY_SIZE] = {0};
static uint8_t acIndex = 0;
static uint8_t acCount = 0;
static bool acLocked = false;
static unsigned long acLockoutUntil = 0;

Macro macQuick[3];

static inline bool isModifierKeycode(uint8_t kc) {
  return kc >= 0xE0 && kc <= 0xE7;
}
static inline uint8_t modifierBitFor(uint8_t kc) {
  return (uint8_t)(1u << (kc - 0xE0));
}

static void waitReportReady() {
  uint8_t guard = 0;
  while (!tud_hid_ready() && guard < 50) {
    delay(1);
    guard++;
  }
}

static void sendReport() {
  uint8_t keycodes[MACRO_MAX_HELD_KEYS] = {0};
  for (uint8_t i = 0; i < heldCount; i++) keycodes[i] = heldKeycodes[i];
  waitReportReady();
  tud_hid_keyboard_report(dev.report_id, heldModifier, keycodes);
}

static void pressKeycode(uint8_t keycode) {
  if (keycode == 0) return;
  if (isModifierKeycode(keycode)) {
    heldModifier |= modifierBitFor(keycode);
    return;
  }
  for (uint8_t i = 0; i < heldCount; i++) {
    if (heldKeycodes[i] == keycode) return;
  }
  if (heldCount >= MACRO_MAX_HELD_KEYS) {
    for (uint8_t i = 1; i < MACRO_MAX_HELD_KEYS; i++) heldKeycodes[i - 1] = heldKeycodes[i];
    heldCount = MACRO_MAX_HELD_KEYS - 1;
  }
  heldKeycodes[heldCount++] = keycode;
}

static void releaseKeycode(uint8_t keycode) {
  if (keycode == 0) return;
  if (isModifierKeycode(keycode)) {
    heldModifier &= (uint8_t)~modifierBitFor(keycode);
    return;
  }
  for (uint8_t i = 0; i < heldCount; i++) {
    if (heldKeycodes[i] == keycode) {
      for (uint8_t j = i; j < heldCount - 1; j++) heldKeycodes[j] = heldKeycodes[j + 1];
      heldCount--;
      heldKeycodes[heldCount] = 0;
      return;
    }
  }
}

void macroReleaseAll() {
  heldCount = 0;
  heldModifier = 0;
  for (uint8_t i = 0; i < MACRO_MAX_HELD_KEYS; i++) heldKeycodes[i] = 0;
  sendReport();
}

bool macroIsRunning() { return macroRunning; }

static bool asciiToKeycode(char c, uint8_t &keycode, bool &shift) {
  shift = false;
  if (c >= 'a' && c <= 'z') { keycode = HID_KEY_A + (c - 'a'); return true; }
  if (c >= 'A' && c <= 'Z') { keycode = HID_KEY_A + (c - 'A'); shift = true; return true; }
  if (c >= '1' && c <= '9') { keycode = HID_KEY_1 + (c - '1'); return true; }
  switch (c) {
    case '0': keycode = HID_KEY_0; return true;
    case ' ': keycode = HID_KEY_SPACE; return true;
    case '\n': keycode = HID_KEY_ENTER; return true;
    case '\t': keycode = HID_KEY_TAB; return true;
    case '-': keycode = HID_KEY_MINUS; return true;
    case '_': keycode = HID_KEY_MINUS; shift = true; return true;
    case '=': keycode = HID_KEY_EQUAL; return true;
    case '+': keycode = HID_KEY_EQUAL; shift = true; return true;
    case '[': keycode = HID_KEY_BRACKET_LEFT; return true;
    case '{': keycode = HID_KEY_BRACKET_LEFT; shift = true; return true;
    case ']': keycode = HID_KEY_BRACKET_RIGHT; return true;
    case '}': keycode = HID_KEY_BRACKET_RIGHT; shift = true; return true;
    case '\\': keycode = HID_KEY_BACKSLASH; return true;
    case '|': keycode = HID_KEY_BACKSLASH; shift = true; return true;
    case ';': keycode = HID_KEY_SEMICOLON; return true;
    case ':': keycode = HID_KEY_SEMICOLON; shift = true; return true;
    case '\'': keycode = HID_KEY_APOSTROPHE; return true;
    case '"': keycode = HID_KEY_APOSTROPHE; shift = true; return true;
    case '`': keycode = HID_KEY_GRAVE; return true;
    case '~': keycode = HID_KEY_GRAVE; shift = true; return true;
    case ',': keycode = HID_KEY_COMMA; return true;
    case '<': keycode = HID_KEY_COMMA; shift = true; return true;
    case '.': keycode = HID_KEY_PERIOD; return true;
    case '>': keycode = HID_KEY_PERIOD; shift = true; return true;
    case '/': keycode = HID_KEY_SLASH; return true;
    case '?': keycode = HID_KEY_SLASH; shift = true; return true;
    case '!': keycode = HID_KEY_1; shift = true; return true;
    case '@': keycode = HID_KEY_2; shift = true; return true;
    case '#': keycode = HID_KEY_3; shift = true; return true;
    case '$': keycode = HID_KEY_4; shift = true; return true;
    case '%': keycode = HID_KEY_5; shift = true; return true;
    case '^': keycode = HID_KEY_6; shift = true; return true;
    case '&': keycode = HID_KEY_7; shift = true; return true;
    case '*': keycode = HID_KEY_8; shift = true; return true;
    case '(': keycode = HID_KEY_9; shift = true; return true;
    case ')': keycode = HID_KEY_0; shift = true; return true;
    default: return false;
  }
}

static void typeText(const char *text, unsigned long interCharDelay) {
  if (!text) return;
  uint8_t empty[MACRO_MAX_HELD_KEYS] = {0};
  for (const char *p = text; *p != '\0'; p++) {
    uint8_t kc;
    bool shift;
    if (!asciiToKeycode(*p, kc, shift)) continue;

    uint8_t report[MACRO_MAX_HELD_KEYS] = {kc, 0, 0, 0, 0, 0};
    uint8_t mod = shift ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;

    waitReportReady();
    tud_hid_keyboard_report(dev.report_id, mod, report);
    delay(interCharDelay);

    waitReportReady();
    tud_hid_keyboard_report(dev.report_id, 0, empty);
    delay(interCharDelay);
  }
}

static bool acCheckAndRecord(uint8_t triggerId) {
  unsigned long now = millis();

  if (acLocked) {
    if (now < acLockoutUntil) return false;
    acLocked = false;
    acCount = 0;
    acIndex = 0;
  }

  acTimestamps[acIndex] = now;
  acTriggerIds[acIndex] = triggerId;
  acIndex = (acIndex + 1) % ANTICHEAT_HISTORY_SIZE;
  if (acCount < ANTICHEAT_HISTORY_SIZE) acCount++;

  if (acCount < ANTICHEAT_MIN_SAMPLES) return true;

  uint8_t start = (acIndex + ANTICHEAT_HISTORY_SIZE - acCount) % ANTICHEAT_HISTORY_SIZE;

  unsigned long minIv = 0xFFFFFFFFUL, maxIv = 0;
  uint8_t nInt = 0;
  unsigned long prevTs = 0;
  bool haveFirst = false;

  for (uint8_t i = 0; i < acCount; i++) {
    uint8_t idx = (start + i) % ANTICHEAT_HISTORY_SIZE;
    unsigned long ts = acTimestamps[idx];
    if (haveFirst) {
      unsigned long iv = ts - prevTs;
      if (iv < minIv) minIv = iv;
      if (iv > maxIv) maxIv = iv;
      nInt++;
    }
    haveFirst = true;
    prevTs = ts;
  }

  bool suspicious = false;

  if (nInt >= ANTICHEAT_MIN_SAMPLES - 1 &&
      minIv < ANTICHEAT_MIN_INTERVAL_MS &&
      (maxIv - minIv) <= ANTICHEAT_MAX_JITTER_MS) {
    suspicious = true;
  }

  if (!suspicious) {
    uint8_t k0 = acTriggerIds[start];
    uint8_t k1 = 0;
    bool foundSecond = false;
    bool pattern = true;
    for (uint8_t i = 0; i < acCount; i++) {
      uint8_t idx = (start + i) % ANTICHEAT_HISTORY_SIZE;
      uint8_t key = acTriggerIds[idx];
      if ((i % 2) == 0) {
        if (key != k0) { pattern = false; break; }
      } else {
        if (!foundSecond) {
          k1 = key;
          foundSecond = true;
          if (k1 == k0) { pattern = false; break; }
        } else if (key != k1) {
          pattern = false;
          break;
        }
      }
    }
    if (pattern && foundSecond && minIv < ANTICHEAT_MIN_INTERVAL_MS * 2) {
      suspicious = true;
    }
  }

  if (suspicious) {
    acLocked = true;
    acLockoutUntil = now + ANTICHEAT_LOCKOUT_MS;
    acCount = 0;
    acIndex = 0;
    return false;
  }

  return true;
}

bool macroIsLocked() {
  if (acLocked && millis() >= acLockoutUntil) return false;
  return acLocked;
}

unsigned long macroLockoutRemainingMs() {
  if (!acLocked) return 0;
  unsigned long now = millis();
  if (now >= acLockoutUntil) return 0;
  return acLockoutUntil - now;
}

bool executeMacro(Macro &m, uint8_t triggerId) {
  if (!acCheckAndRecord(triggerId)) return false;
  if (m.macCount <= 0) return false;

  macroRunning = true;

  for (int i = 0; i < m.macCount; i++) {
    macroAct &act = m.actions[i];

    switch (act.mType) {
      case MACRO_PRESS: {
        uint8_t savedMod = heldModifier;
        heldModifier |= act.modifier;
        pressKeycode(act.keycode);
        sendReport();
        delay(MACRO_TAP_HOLD_MS);
        releaseKeycode(act.keycode);
        heldModifier = savedMod;
        sendReport();
        delay(act.actDelay);
        break;
      }

      case MACRO_HOLD: {
        heldModifier |= act.modifier;
        pressKeycode(act.keycode);
        sendReport();
        delay(act.actDelay);
        break;
      }

      case MACRO_RELEASE: {
        releaseKeycode(act.keycode);
        heldModifier &= (uint8_t)~act.modifier;
        sendReport();
        delay(act.actDelay);
        break;
      }

      case MACRO_TEXT: {
        macroReleaseAll();
        typeText(act.text, act.actDelay);
        break;
      }

      case MACRO_DELAY: {
        delay(act.actDelay);
        break;
      }
    }
  }

  macroReleaseAll();
  macroRunning = false;
  return true;
}

bool addAct(Macro &m, uint8_t keycode, macType mt, unsigned long adl, uint8_t modifier) {
  if (m.macCount < 0 || m.macCount >= MACRO_MAX_ACTIONS) return false;
  macroAct &act = m.actions[m.macCount++];
  act.keycode = keycode;
  act.modifier = modifier;
  act.text[0] = '\0';
  act.actDelay = adl;
  act.mType = mt;
  return true;
}

bool insertAct(Macro &m, int index, uint8_t keycode, macType mt, unsigned long adl, uint8_t modifier) {
  if (index < 0 || index > m.macCount || m.macCount >= MACRO_MAX_ACTIONS) return false;
  for (int i = m.macCount; i > index; i--) m.actions[i] = m.actions[i - 1];
  m.macCount++;
  macroAct &act = m.actions[index];
  act.keycode = keycode;
  act.modifier = modifier;
  act.text[0] = '\0';
  act.actDelay = adl;
  act.mType = mt;
  return true;
}

bool editAct(Macro &m, int index, uint8_t keycode, macType mt, unsigned long adl, uint8_t modifier) {
  if (index < 0 || index > m.macCount - 1) return false;
  macroAct &act = m.actions[index];
  act.keycode = keycode;
  act.modifier = modifier;
  act.actDelay = adl;
  act.mType = mt;
  if (mt != MACRO_TEXT) act.text[0] = '\0';
  return true;
}

bool removeAct(Macro &m, int index) {
  if (index < 0 || index >= m.macCount) return false;
  for (int i = index; i < m.macCount - 1; i++) m.actions[i] = m.actions[i + 1];
  m.macCount--;
  return true;
}

bool clearMacro(Macro &m) {
  m.macCount = 0;
  return true;
}

static void copyText(macroAct &act, const char *text) {
  if (!text) text = "";
  uint8_t i = 0;
  for (; i < MACRO_TEXT_MAX_LEN - 1 && text[i] != '\0'; i++) act.text[i] = text[i];
  act.text[i] = '\0';
}

bool addTextAct(Macro &m, const char *text, unsigned long adl) {
  if (m.macCount < 0 || m.macCount >= MACRO_MAX_ACTIONS) return false;
  macroAct &act = m.actions[m.macCount++];
  act.keycode = 0;
  act.modifier = 0;
  act.mType = MACRO_TEXT;
  act.actDelay = adl;
  copyText(act, text);
  return true;
}

bool insertTextAct(Macro &m, int index, const char *text, unsigned long adl) {
  if (index < 0 || index > m.macCount || m.macCount >= MACRO_MAX_ACTIONS) return false;
  for (int i = m.macCount; i > index; i--) m.actions[i] = m.actions[i - 1];
  m.macCount++;
  macroAct &act = m.actions[index];
  act.keycode = 0;
  act.modifier = 0;
  act.mType = MACRO_TEXT;
  act.actDelay = adl;
  copyText(act, text);
  return true;
}

bool editTextAct(Macro &m, int index, const char *text, unsigned long adl) {
  if (index < 0 || index > m.macCount - 1) return false;
  macroAct &act = m.actions[index];
  act.keycode = 0;
  act.modifier = 0;
  act.mType = MACRO_TEXT;
  act.actDelay = adl;
  copyText(act, text);
  return true;
}

bool addDelayAct(Macro &m, unsigned long adl) {
  return addAct(m, 0, MACRO_DELAY, adl, 0);
}

bool macroHasUnbalancedHolds(const Macro &m) {
  uint8_t pending[MACRO_MAX_ACTIONS];
  uint8_t pendingCount = 0;

  for (int i = 0; i < m.macCount; i++) {
    const macroAct &act = m.actions[i];
    if (act.mType == MACRO_HOLD) {
      if (pendingCount < MACRO_MAX_ACTIONS) pending[pendingCount++] = act.keycode;
    } else if (act.mType == MACRO_RELEASE) {
      for (uint8_t j = 0; j < pendingCount; j++) {
        if (pending[j] == act.keycode) {
          for (uint8_t k = j; k < pendingCount - 1; k++) pending[k] = pending[k + 1];
          pendingCount--;
          break;
        }
      }
    }
  }

  return pendingCount > 0;
}

#define MACRO_FILE_PT 10

bool saveMacro(const char *path, Macro &m) {
  File file = LittleFS.open(path, "w");
  if (!file) return false;

  DynamicJsonDocument doc(4096);
  doc["pt"] = MACRO_FILE_PT;
  doc["ver"] = 1;

  JsonArray acts = doc.createNestedArray("ac");
  for (int i = 0; i < m.macCount && i < MACRO_MAX_ACTIONS; i++) {
    const macroAct &act = m.actions[i];
    JsonObject obj = acts.createNestedObject();
    obj["t"] = (uint8_t)act.mType;
    obj["k"] = act.keycode;
    obj["m"] = act.modifier;
    obj["d"] = act.actDelay;
    if (act.mType == MACRO_TEXT) obj["x"] = act.text;
  }

  bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

bool loadMacro(const char *path, Macro &m) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;
  if (!doc["pt"].is<int>() || doc["pt"].as<int>() != MACRO_FILE_PT) return false;

  clearMacro(m);

  if (doc["ac"].is<JsonArray>()) {
    for (JsonObject obj : doc["ac"].as<JsonArray>()) {
      if (m.macCount >= MACRO_MAX_ACTIONS) break;

      uint8_t rawType = obj["t"] | (uint8_t)MACRO_PRESS;
      if (rawType > MACRO_DELAY) continue;

      macType mt      = (macType)rawType;
      uint8_t kc       = obj["k"] | (uint8_t)0;
      uint8_t md       = obj["m"] | (uint8_t)0;
      unsigned long adl = obj["d"] | 0UL;

      if (mt == MACRO_TEXT) {
        const char *txt = obj["x"] | "";
        addTextAct(m, txt, adl);
      } else {
        addAct(m, kc, mt, adl, md);
      }
    }
  }

  return true;
}