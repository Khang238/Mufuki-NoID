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

const int KB_ROW_COUNT = 5;

const char* kbCharsLower[4] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl",
  "zxcvbnm"
};
const char* kbCharsUpper[4] = {
  "1234567890",
  "QWERTYUIOP",
  "ASDFGHJKL",
  "ZXCVBNM"
};
const char* kbSymbols[4] = {
  "!@#$%^&*()",
  "-_=+[]{}\\|",
  ";:'\",.<>/?",
  "`~"
};
const char* kbFuncLabels[4]    = {"Shift", "123", "Space", "Enter"};
const char* kbFuncLabelsSym[4] = {"Shift", "ABC", "Space", "Enter"};

int kbRowLen(int page, int row) {
  if (row == 4) return 4;
  const char* r = (page == 0) ? kbCharsLower[row] : kbSymbols[row];
  return strlen(r);
}

char kbGetChar(int page, int row, int col, bool shiftOn) {
  if (page == 0) return shiftOn ? kbCharsUpper[row][col] : kbCharsLower[row][col];
  return kbSymbols[row][col];
}

String kbGetLabel(int page, int row, int col, int shiftState) {
  if (row == 4) {
    const char* lbl = (page == 0 ? kbFuncLabels[col] : kbFuncLabelsSym[col]);
    if (col == 0) {
      if (shiftState == 2) return "CAPS";
      if (shiftState == 1) return "SHFT";
      return "shift";
    }
    return String(lbl);
  }
  return String(kbGetChar(page, row, col, shiftState != 0));
}

void drawSingleLineTruncated(String text, bool showCursor, int y) {
  String display = text;

  if (u8g2.getStrWidth(display.c_str()) <= 128) {
    if (showCursor) display += "_";
    u8g2.drawStr(0, y, display.c_str());
    return;
  }

  String prefix = "...";
  String tail = display;
  while (tail.length() > 0 &&
         u8g2.getStrWidth((prefix + tail).c_str()) > 128) {
    tail.remove(0, 1);
  }
  tail.remove(0, 1);
  if (showCursor) tail += "_";
  u8g2.drawStr(0, y, (prefix + tail).c_str());
}

String qwertyKeyboard(String text) {
  String prevText = text;
  bool typing = true;
  int page = 0;
  int curRow = 1;
  int curCol = 0;
  int shiftState = 0;
  unsigned long lastShiftTap = 0;
  while (getButton(true) > 0) delay(10);
  while (typing) {

    int button = getButton();
    if (button == 0) {
      curRow = (curRow - 1 + KB_ROW_COUNT) % KB_ROW_COUNT;
      int len = kbRowLen(page, curRow);
      if (curCol >= len) curCol = len - 1;
    }
    if (button == 1) {
      if (curRow == 4) {
        if (curCol == 0) {
          if (millis() - lastShiftTap < 400) {
            shiftState = (shiftState == 2) ? 0 : 2;
          } else {
            shiftState = (shiftState == 0) ? 1 : 0;
          }
          lastShiftTap = millis();
        } else if (curCol == 1) {
          page = 1 - page;
        } else if (curCol == 2) {
          if (text.length() < 31) text += ' ';
        } else if (curCol == 3) {
          typing = false;
        }
      } else {
        char c = kbGetChar(page, curRow, curCol, shiftState != 0);
        if (text.length() < 31) text += c;
        if (shiftState == 1) shiftState = 0;
      }
    }
    if (button == 2) {
      curRow = (curRow + 1) % KB_ROW_COUNT;
      int len = kbRowLen(page, curRow);
      if (curCol >= len) curCol = len - 1;
    }
    if (button == 3) {
      globFont();
      while (getButton(true) > 0) delay(10);
      return prevText;
    }

    if (applyEffect[0]) {
      int len = kbRowLen(page, curRow);
      curCol = (curCol - 1 + len) % len;
      applyEffect[0] = false;
    }
    if (applyEffect[1]) {
      int len = kbRowLen(page, curRow);
      curCol = (curCol + 1) % len;
      applyEffect[1] = false;
    }
    if (applyEffect[2]) {
      if (text.length() > 0) text.remove(text.length() - 1);
      applyEffect[2] = false;
    }

    u8g2.clearBuffer();
    globFont();
    drawSingleLineTruncated(text, ((millis() % 1000) < 500), 9);

    u8g2.setFont(u8g2_font_5x7_tr);
    int rowY[KB_ROW_COUNT] = {24, 32, 40, 48, 58};
    for (int r = 0; r < KB_ROW_COUNT; r++) {
      int len = kbRowLen(page, r);
      int cellW = (r < 4) ? (128 / 10) : (128 / 4);
      for (int c = 0; c < len; c++) {
        int x = c * cellW;
        String lbl = kbGetLabel(page, r, c, shiftState);
        if (r == curRow && c == curCol) {
          u8g2.drawBox(x, rowY[r] - 7, cellW - 1, 9);
          u8g2.setDrawColor(0);
          u8g2.drawStr(x + 1, rowY[r], lbl.c_str());
          u8g2.setDrawColor(1);
        } else {
          u8g2.drawStr(x + 1, rowY[r], lbl.c_str());
        }
      }
    }
    u8g2.sendBuffer();
  }

  globFont();
  while (getButton(true) > 0) delay(10);
  return text;
}

String keyboard(String text) {
  String prevText = text;
  if (morseKey) {
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
          if (text.length() < 31 && !unsignedCharacter)
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
  } else return qwertyKeyboard(text);
  root = {'\0', nullptr, nullptr};
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
  if (alwaysReport) needReport = true;
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