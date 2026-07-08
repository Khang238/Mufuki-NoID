#include "input.h"

int overSample(int chan, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(adcPins[chan]);
  }
  return sum / samples;
}

float y[] = {0.0, 0.0, 0.0};
int expoMovAvr(int chan, float alpha) {
  float x = analogRead(adcPins[chan]);
  y[chan] = y[chan] + alpha * (x - y[chan]);
  return y[chan];
}

void readHall(int i) {
  if (prf.doFilter) {
    if (prf.filterType == 0) rawVal[i] = overSample(i, prf.ovsSamples);
    else rawVal[i] = expoMovAvr(i, prf.emaAlpha);
  } else rawVal[i] = analogRead(adcPins[i]);
  hallVal[i] = constrain(
    (float)(rawVal[i] - prf.calMin[i] - prf.deadZone[i]) /
    (float)(prf.calMax[i] - prf.calMin[i] - 2 * prf.deadZone[i]),
    0.00, 1.00
  );
}

void inputTypeDigitalEmulation() {
  for (int i = 0; i < 6; i++) {
    if (i < 3) {
      readHall(i);
      nowPress[i] = (hallVal[i] > prf.actuation);
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
      if (hallVal[i] > prf.upperThreshold)
        nowPress[i] = true;
      else if (hallVal[i] < prf.lowerThreshold)
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
      if (hallVal[i] > windowFoot[i] + prf.windowSize) {
        nowPress[i] = true;
        windowFoot[i] = hallVal[i] - prf.windowSize;
      }
      else if (hallVal[i] < windowFoot[i]) {
        nowPress[i] = false;
        windowFoot[i] = hallVal[i];
      }
      if (hallVal[i] == 0.0) windowFoot[i] = 0.0;
      if (hallVal[i] == 1.0) windowFoot[i] = 1.0 - prf.windowSize;
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
  switch (prf.inputHandler) {
    case 0: inputTypeDigitalEmulation(); break;
    case 1: inputTypeHysteresisHandling(); break;
    case 2: inputTypeDynamicActuation(); break;
    default: inputTypeDigitalEmulation(); break;
  }
}

int getButton(bool hold) {
  for (int i = 0; i < 4; i++) {
    if (!digitalRead(btnPins[i])) {
      if (hold) return i;
      if (!holding[i]) {
        pressTime[i] = millis();
        holding[i] = true;
        return i;
      }
    } else {
      if (holding[i]) {
        holding[i] = false;
      }
    }
  }
  return -1;
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
      // updateInput();
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
      u8g2.setFont(u8g2_font_spleen16x32_mr);
      u8g2.drawStr(64 - u8g2.getStrWidth(currentCode.c_str()) / 2, 20, currentCode.c_str());
      globFont();
      char tmp = decodeMorse(currentCode.c_str());
      String tmpStr = unsignedCharacter ? "[???]" : String(tmp);
      if (currentCode == "..--") tmpStr = "[space]";
      else if (currentCode == ".-.-") tmpStr = "[enter]";
      else if (currentCode == "----") tmpStr = "[backspace]";
      else if (currentCode == "....-.") tmpStr = "[caps " + String(capsLock ? "off" : "on") + "]";
      if (currentCode != "") u8g2.drawStr(64 - u8g2.getStrWidth(tmpStr.c_str()) / 2, 32, tmpStr.c_str());
      else u8g2.drawStr(64 - u8g2.getStrWidth("[none]") / 2, 32, "[none]");
      u8g2.drawStr(0, 40, String((String)"Text: " + (capsLock ? "[C]" : "")).c_str());
      drawWrappedText(u8g2, 0, 50, 128, (((millis() - lastInputTime + 251) % 500 < 250) ? text : text + "_").c_str());
      u8g2.sendBuffer();
    }
  }
  root = {'\0', nullptr, nullptr}; // destroy the tree to save memory
  return text;
}

void keypadMUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_tenthinguys_tf);
  // °
  u8g2.drawLine(64, 0, 64, 34);
  u8g2.drawLine(0, 18, 128, 18);
  u8g2.drawLine(0, 34, 128, 34);
  String tmp = String(temperatureRead(), 1) + String((char)0xB0) + "C";
  u8g2.drawStr(32 - u8g2.getStrWidth(tmp.c_str()) / 2, 15, tmp.c_str());
  tmp = String(lastRate) + "rps";
  u8g2.drawStr(96 - u8g2.getStrWidth(tmp.c_str()) / 2, 15, tmp.c_str());
  tmp = withBLE ? "BLE Key" : "Keypad" ;
  u8g2.drawStr(32 - u8g2.getStrWidth(tmp.c_str()) / 2, 30, tmp.c_str());

  switch (prf.inputHandler) {
    case 0: tmp = "DEM" ; break;
    case 1: tmp = "HSR"  ; break;
    case 2: tmp = "DAC"  ; break;
  }
  u8g2.drawStr(96 - u8g2.getStrWidth(tmp.c_str()) / 2, 30, tmp.c_str());

  if (prf.inputHandler == 1) tmp = "Act: +" + String(prf.upperThreshold * (prf.hallDisplayAsKT ? prf.keyTravel : 1), 2) + " -" + String(prf.lowerThreshold * (prf.hallDisplayAsKT ? prf.keyTravel : 1), 2);
  else                   tmp = "Act: " + String(prf.inputHandler == 0 ? prf.actuation * (prf.hallDisplayAsKT ? prf.keyTravel : 1) : prf.windowSize * (prf.hallDisplayAsKT ? prf.keyTravel : 1), 2) +  (prf.hallDisplayAsKT ? "mm" : "");
  // if (withBLE && !kblue->isConnected()) u8g2.drawStr(64 - u8g2.getStrWidth("Not Connected") / 2, 48, "Not Connected");
  // else u8g2.drawStr(64 - u8g2.getStrWidth(tmp.c_str()) / 2, 48, tmp.c_str());
  u8g2.drawStr(64 - u8g2.getStrWidth(tmp.c_str()) / 2, 48, tmp.c_str());

  //float maxPress = 0.00;
  u8g2.setFont(u8g2_font_5x8_tr);
  for (int i = 0; i < 3; i++) {
    if (hallVal[i] > 0.05) waitIDLE = millis();
    u8g2.drawFrame(43 * i - 1, 55, 44, 10);
    u8g2.drawBox(43 * i, 55, (int)(hallVal[i] * 43), 10);
    u8g2.setDrawColor(2);
    String line = prf.hallDisplayAsKT ? (String(hallVal[i] * prf.keyTravel, 1) + "mm") : String(hallVal[i], 2);
    u8g2.drawStr(1 + 43 * i, 63, line.c_str());
    u8g2.setDrawColor(1);
    //maxPress = max(maxPress, hallVal[i]);
  }
  u8g2.sendBuffer();
}

void handleKeypad() {
  // Input Handling
  if (alwaysReport) needReport = true;

  // USB Report
  if (needReport) {
    uint8_t keycodes[6] = {0};
    uint8_t idx = 0;
    for (int i = 0; i < 6; i++)
      if (nowPress[i]) keycodes[idx++] = prf.layout[i];
    tud_hid_keyboard_report(dev.report_id, 0, keycodes);
  }
}

void mouseMUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
  u8g2.drawStr(0, 18, withBLE ? "Mode: BLE Mouse" : "Mode: Mouse");
  u8g2.drawStr(0, 26, ("update rate: " + String(lastRate) + "r/s").c_str());
  // if (withBLE && !kblue->isConnected()) u8g2.drawStr(0, 34, "Not Connected");
  u8g2.drawFrame(4, 30, 32, 32); // mouse direction
  int centerX = 20, centerY = 46;
  int cursorX = centerX + (int)(mos.mouseX * 16 / 127);
  int cursorY = centerY + (int)(mos.mouseY * 16 / 127);
  u8g2.drawLine(centerX, centerY, cursorX, cursorY);
  u8g2.drawBox(cursorX - 1, cursorY - 1, 3, 3);
  // wheel
  u8g2.drawFrame(40, 30, 8, 32);
  if (mos.mouseWheel != 0) {
    int wheelY = 46 - (int)(mos.mouseWheel * 14 / 16);
    u8g2.drawLine(44, 46, 44, wheelY);
    u8g2.drawBox(43, wheelY - 1, 3, 3);
  }
  u8g2.sendBuffer();
}

void handleMouse() {
  applyMappings(prf, mos);
  tud_hid_mouse_report(mdev.report_id, mos.mouseButtons, -mos.mouseX, -mos.mouseY, mos.mouseWheel, 0);
  // mdev.move(mos.mouseX, mos.mouseY);
}

void gamepadMUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
  u8g2.drawStr(0, 18, withBLE ? "Mode: BLE Gamepad" : "Mode: Gamepad");
  u8g2.drawStr(0, 26, ("update rate: " + String(lastRate) + "r/s").c_str());
  // if (withBLE && !kblue->isConnected()) u8g2.drawStr(0, 34, "Not Connected");
  u8g2.drawFrame(4, 30, 24, 24); // left stick
  int centerX = 16, centerY = 42;
  int cursorX = centerX + (int)(mos.axes[0] * 12 / 127);
  int cursorY = centerY + (int)(mos.axes[1] * 12 / 127);
  u8g2.drawLine(centerX, centerY, cursorX, cursorY);
  u8g2.drawBox(cursorX - 1, cursorY - 1, 3, 3);
  u8g2.drawFrame(100, 30, 24, 24); // right stick
  centerX = 112; centerY = 42;
  cursorX = centerX + (int)(mos.axes[2] * 12 / 127);
  cursorY = centerY + (int)(mos.axes[3] * 12 / 127);
  u8g2.drawLine(centerX, centerY, cursorX, cursorY);
  u8g2.drawBox(cursorX - 1, cursorY - 1, 3, 3);
  // triggers
  u8g2.drawFrame(4, 58, 24, 6); // left trigger
  int fillWidth = (int)(mos.axes[4] * 24 / 127);
  if (fillWidth > 0) u8g2.drawBox(4, 58, fillWidth, 6);
  u8g2.drawFrame(100, 58, 24, 6); // right trigger
  fillWidth = (int)(mos.axes[5] * 24 / 127);
  if (fillWidth > 0) u8g2.drawBox(100, 58, fillWidth, 6);
  // buttons
  for (int y = 0; y < 2; y++) {
    for (int x = 0; x < 8; x++) {
      int idx = y * 8 + x;
      u8g2.drawFrame(34 + x * 8, 30 + y * 8, 6, 6);
      if (mos.gpButtons & (1 << idx))
        u8g2.drawBox(34 + x * 8, 30 + y * 8, 6, 6);
    }
  }
  u8g2.sendBuffer();
}

void handleGamepad() {
  applyMappings(prf, mos);
  gdev.sendAll(
    mos.gpButtons,    // buttons (uint32_t)
    mos.axes[0],      // x  = LX
    mos.axes[1],      // y  = LY
    mos.axes[2],      // z  = RX
    mos.axes[3],      // rz = RY
    mos.axes[4],      // rx = LT
    mos.axes[5],      // ry = RT
    0                   // hat (D-pad, not used → 0)
  );
}