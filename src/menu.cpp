#include "menu.h"

float graphData[GRAPH_WIDTH];
int graphIndex = 0;

void pushGraphValue(float val) {
  int y = (int)(val * (GRAPH_HEIGHT - 1));
  graphData[graphIndex] = y;
  graphIndex = (graphIndex + 1) % GRAPH_WIDTH;
}

void drawGraph(int x, int y) {
  u8g2.drawFrame(x, y, GRAPH_WIDTH, GRAPH_HEIGHT);

  int prevY = -1;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    int bufIndex = (graphIndex + i) % GRAPH_WIDTH;
    int valY = graphData[bufIndex];

    int drawX = x + GRAPH_WIDTH - 1 - i;
    int drawY = y + GRAPH_HEIGHT - 1 - valY;

    if (prevY >= 0) {
      u8g2.drawLine(drawX + 1, prevY, drawX, drawY);
    }
    prevY = drawY;
  }
}

float valueSet(const char *title, float input, bool clamp, float clampMin, float clampMax, bool sInt) {
  #define SMTH_WAIT 150
  #define SMTH_FACC 1500
  #define SMTH_FAHH 5000
  #define FTNDZ 0.4

  unsigned long htA = 0, htB = 0, adw = 0;
  bool hlA = false, hlB = false;
  float sVal = input;
  while (!digitalRead(btnPins[2])) delay(10);
  while (true) {
    // updateInput();
    if (digitalRead(btnPins[0])) hlA = false;
    if (digitalRead(btnPins[2])) hlB = false;
    if (!digitalRead(btnPins[0]) && !digitalRead(btnPins[2])) {}
    else if (!digitalRead(btnPins[0])) {
      if (!hlA) {htA = millis(); hlA = true;}
      if (millis() - adw > SMTH_WAIT) {
        adw = millis() + ((millis() - htA < SMTH_FACC) ? 0 : SMTH_WAIT);
        input += (millis() - htA < SMTH_FAHH ? (sInt ? 1 : 0.01) : (sInt ? 8 : 0.81));
      }
    }
    else if (!digitalRead(btnPins[2])) {
      if (!hlB) {htB = millis(); hlB = true;}
      if (millis() - adw > SMTH_WAIT) {
        adw = millis() + ((millis() - htB < SMTH_FACC) ? 0 : SMTH_WAIT);
        input -= (millis() - htB < SMTH_FAHH ? (sInt ? 1 : 0.01) : (sInt ? 8 : 0.81));
      }
    }
    float diff = hallVal[2] - hallVal[0];
    if (fabs(diff) < FTNDZ) diff = 0;
    else if (diff > 0) diff -= FTNDZ;
    else diff += FTNDZ;
    input += diff; // fine-tune with hall input
    if (nowPress[1] || sInt) input = round(input); // snap to integer
    if (clamp) input = constrain(input, clampMin, clampMax);
    u8g2.clearBuffer();
    globFont();
    if (!digitalRead(btnPins[1])) {while (!digitalRead(btnPins[1])) delay(10); return input;}
    if (!digitalRead(btnPins[3])) {while (!digitalRead(btnPins[3])) delay(10); return sVal;}

    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 12, title);
    bool toNeg = diff < 0;
    int smth = (int)((diff) * 40);
    if (fabs(hallVal[2] - hallVal[0]) > FTNDZ) {
      u8g2.drawStr(64 + (int)(diff * 46) - 3, 52, toNeg ? "-" : "+");
      if (toNeg) u8g2.drawBox(64 + smth, 44, abs(smth), 8); 
      else       u8g2.drawBox(64, 44, abs(smth), 8);
    }
    String tmp = String(input, sInt ? 0 : 2);
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 40, tmp.c_str());
    u8g2.setFont(u8g2_font_5x8_tr);
    if (clamp) {
      tmp = "Range: " + String(clampMin, sInt ? 0 : 2) + " to " + String(clampMax, sInt ? 0 : 2);
    } else {
      tmp = "[no limit]";
    }
    u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 64, tmp.c_str());
    u8g2.sendBuffer();
  }
}

int countItems(const char* items) {
  int count = 1;
  while (*items) {
    if (*items == '\n')
      count++;
    items++;
  }
  return count;
}

bool getItem(const char* items, int index, char* out, size_t outSize) {
  int current = 0;

  while (*items && current < index) {
    if (*items == '\n')
      current++;
    items++;
  }

  if (!*items)
    return false;

  size_t i = 0;

  while (*items && *items != '\n' && i < outSize - 1) {
    out[i++] = *items++;
  }

  out[i] = '\0';
  return true;
}

char globalTitle[32] = "NoID_Mufuki";
float globalTitleFloat[32] = {0};
bool isTitleInitialized = false;

void drawScrambleText(int x, int y, const char* targetTitle) {
  if (targetTitle == nullptr) targetTitle = "";
  int targetLen = strlen(targetTitle);
  if (targetLen > 31) targetLen = 31;

  if (!isTitleInitialized) {
    memset(globalTitle, ' ', 31);
    globalTitle[31] = '\0';
    for (int i = 0; i < 32; i++) {
      globalTitleFloat[i] = 32.0f;
    }
    isTitleInitialized = true;
  }

  int currentLen = strlen(globalTitle);
  int maxLen = (targetLen > currentLen) ? targetLen : currentLen;
  if (maxLen > 31) maxLen = 31;

  bool isFinished = true;
  for (int i = 0; i < maxLen; i++) {
    float tar = (i < targetLen) ? (float)targetTitle[i] : 32.0f;
    
    globalTitleFloat[i] += (tar - globalTitleFloat[i]) * 0.25f;
    
    int asciiVal = (int)(globalTitleFloat[i] + 0.5f);
    if (asciiVal < 32) asciiVal = 32;
    if (asciiVal > 126) asciiVal = 126;
    
    globalTitle[i] = (char)asciiVal;

    if (abs(tar - globalTitleFloat[i]) > 0.1f) {
      isFinished = false;
    }
  }
  globalTitle[maxLen] = '\0';

  u8g2.drawStr(x, y, globalTitle);
}

float windowPos = 0;
int noidMenu(const char* title, int startIndex, const char* list, bool rlb) {
  #define COLLAPSE_SPACE 6
  #define USE_U8G2_MENU  0

  while (getButton(true) != -1) {delay(10);}
  if (USE_U8G2_MENU) return u8g2.userInterfaceSelectionList(title, startIndex, list);
  if (startIndex > 0) startIndex--;
  int selected = startIndex;
  
  int count = countItems(list);
  int fontH = u8g2.getMaxCharHeight();
  int boxH = fontH + 2;
  
  const char* targetTitle = (title != nullptr) ? title : "";
  int targetLen = strlen(targetTitle);
  if (targetLen > 31) targetLen = 31;

  int titleH = (targetLen > 0) ? (fontH + 4) : 0;
  int viewH = 64 - titleH;

  float selBox = selected * boxH;

  windowPos = rlb ? windowPos : selBox;

  float maxWindowPos = (count * boxH > viewH) ? (count * boxH - viewH) : 0;

  float lift = 0;
  const float maxLift = 10;

  unsigned long hldTimer = 0;
  unsigned long movTimer = 0;
  int movDir = 0;
  bool hld = false;

  const int contentWidth = 122;
  const int sbX = 124;
  const int sbW = 4;

  if (!isTitleInitialized) {
    for (int i = 0; i < 32; i++) {
      globalTitleFloat[i] = (float)globalTitle[i];
    }
    isTitleInitialized = true;
  }

  while (true) {
    int btn = getButton(true);
    if (btn == 3) {while (getButton(true) == 3) delay(10); return 0;}
    if (btn == 1) {while (getButton(true) == 1) delay(10); return selected + 1;}
    if (!hld) {
      if (btn == 0) {
        hld = true;
        hldTimer = millis(); 
        movDir = -1;
        selected = (selected - 1 + count) % count; lift = 0;
      }
      else if (btn == 2) {
        hld = true;
        hldTimer = millis(); 
        movDir = 1;
        selected = (selected + 1) % count; lift = 0;
      }
    }
    if (btn == -1) {hld = false; movDir = 0;}

    if (hld && millis() - hldTimer > SMTH_FACC) {
      if (millis() - movTimer > SMTH_WAIT) {
        if (movDir == -1) {selected = (selected - 1 + count) % count; lift = 0;}
        else if (movDir == 1) {selected = (selected + 1) % count; lift = 0;}
        if (millis() - hldTimer < SMTH_FAHH) movTimer = millis();
      }
    }

    float targetSelBox = selected * boxH;
    selBox += (targetSelBox - selBox) * 0.5; 

    float targetWindowPos = windowPos;
    if (targetSelBox < windowPos) {
      targetWindowPos = targetSelBox;
    } else if (targetSelBox + boxH > windowPos + viewH) {
      targetWindowPos = targetSelBox + boxH - viewH;
    }
    if (targetWindowPos < 0) targetWindowPos = 0;
    if (targetWindowPos > maxWindowPos) targetWindowPos = maxWindowPos;

    windowPos += (targetWindowPos - windowPos) * 0.5;
    lift += (maxLift - lift) * 0.25;

    // if (titleH > 0) {
    //   int currentLen = strlen(globalTitle);
    //   int maxLen = (targetLen > currentLen) ? targetLen : currentLen;
    //   if (maxLen > 31) maxLen = 31;
    //   for (int i = 0; i < maxLen; i++) {
    //     float tar = (i < targetLen) ? (float)targetTitle[i] : 32.0f;
    //     globalTitleFloat[i] += (tar - globalTitleFloat[i]) * 0.25;
    //     int asciiVal = (int)(globalTitleFloat[i] + 0.5f);
    //     if (asciiVal < 32) asciiVal = 32;
    //     if (asciiVal > 126) asciiVal = 126;
    //     globalTitle[i] = (char)asciiVal;
    //   }
    //   globalTitle[maxLen] = '\0';
    // }

    u8g2.clearBuffer();
    for (int i = 0; i < count; i++) {
      float itemY = titleH + (i * boxH) - windowPos;
      if (itemY + boxH >= titleH && itemY <= 64) {
        char buf[32];
        getItem(list, i, buf, sizeof(buf));
        if (i == selected) {
          u8g2.drawStr(4 + lift, itemY + fontH - 1, buf);
        } else {
          u8g2.drawStr(4, itemY + fontH - 1, buf);
        }
      }
    }
    
    u8g2.setDrawColor(2); 
    u8g2.drawRBox(0, titleH + (selBox - windowPos), contentWidth, boxH, 1);
    
    u8g2.setDrawColor(1);
    int totalContentH = count * boxH;
    if (totalContentH > viewH) {
      int sbH = (viewH * viewH) / totalContentH;
      if (sbH < 4) sbH = 4;
      
      int sbY = titleH + ((viewH - sbH) * windowPos) / maxWindowPos;
      
      u8g2.drawVLine(sbX + sbW / 2, titleH, viewH);
      u8g2.drawRBox(sbX, sbY, sbW, sbH, 1);
    }

    u8g2.setDrawColor(0); 
    u8g2.drawBox(0, 0, 128, titleH - 2);
    u8g2.setDrawColor(1);
    
    if (titleH > 0) {
      u8g2.setDrawColor(1);
      u8g2.setFontMode(1);
      // u8g2.drawStr(4, fontH, globalTitle);
      drawScrambleText(4, fontH, targetTitle);
      u8g2.drawHLine(0, titleH - 2, 128);
    }
    u8g2.sendBuffer();
  }
}

void calibMenu() {
  bool running = true;
  bool calib = false;
  bool calibed = false; 
  int nowCal = 0;
  int calLast1s = 0;
  unsigned long idk = 0;
  int calStep = 0; 

  if (analogLed) {
    ledcWrite(0, 2); ledcWrite(1, 0); ledcWrite(2, 0);
  } else {
    b.clear();
    b.setPixelColor(0, b.Color(255, 255, 255));
    b.show();
  }
  
  for (int i = 0;  i < GRAPH_WIDTH; i++) graphData[i] = {0};

  while (running) {
    if (firstTime) updateInput();
    if (calib) {
      if (calStep == 0) {
        prf.calMin[nowCal] = (prf.calMin[nowCal] < rawVal[nowCal]) ? prf.calMin[nowCal] : rawVal[nowCal];
      } else if (calStep == 1) {
        prf.calMax[nowCal] = (prf.calMax[nowCal] > rawVal[nowCal]) ? prf.calMax[nowCal] : rawVal[nowCal];
      }
    }

    switch (getButton()) {
    case 0:
      nowCal = (nowCal + 2) % 3;
      for (int i = 0;  i < GRAPH_WIDTH; i++) graphData[i] = {0};
      idk = millis();
      calLast1s = rawVal[nowCal];
      calibed = false; 
      calStep = 0;

      for (int i = 0; i < 3; i++) {
        if (i == nowCal) {
          if (analogLed) ledcWrite(i, 2);
          else b.setPixelColor(i, calib ? b.Color(255, 255, 0) : b.Color(255, 255, 255));
        } else {
          if (analogLed) ledcWrite(i, 0);
          else b.setPixelColor(i, b.Color(0, 0, 0));
        }
      }
      if (calib) { prf.calMin[nowCal] = 4095; prf.calMax[nowCal] = 0; }
      break;

    case 1:
      calib = !calib;
      idk = millis();
      calLast1s = rawVal[nowCal];
      calibed = false;
      calStep = 0;

      if (calib) {
        prf.calMin[nowCal] = 4095;
        prf.calMax[nowCal] = 0;
        if (analogLed) ledcWrite(nowCal, 2);
        else b.setPixelColor(nowCal, b.Color(255, 255, 0));
      } else {
        if (analogLed) ledcWrite(nowCal, 2);
        else b.setPixelColor(nowCal, b.Color(255, 255, 255));
      }
      break;

    case 2:
      nowCal = (nowCal + 1) % 3;
      for (int i = 0;  i < GRAPH_WIDTH; i++) graphData[i] = {0};
      
      idk = millis();
      calLast1s = rawVal[nowCal];
      calibed = false;
      calStep = 0;

      for (int i = 0; i < 3; i++) {
        if (i == nowCal) {
          if (analogLed) ledcWrite(i, 2);
          else b.setPixelColor(i, calib ? b.Color(255, 255, 0) : b.Color(255, 255, 255));
        } else {
          if (analogLed) ledcWrite(i, 0);
          else b.setPixelColor(i, b.Color(0, 0, 0));
        }
      }
      if (calib) { prf.calMin[nowCal] = 4095; prf.calMax[nowCal] = 0; }
      break;

    case 3:
      running = false;
      break;
    default:
      break;
    }

    if (calib && calStep < 2) {
      if (millis() - idk > 1000) {
        if (abs(rawVal[nowCal] - calLast1s) <= prf.deadZone[nowCal] * 2) {
          if (calStep == 0 && !calibed) {
            calibed = true;
            if (analogLed) ledcWrite(nowCal, 4);
            else b.setPixelColor(nowCal, b.Color(0, 255, 0));
            calStep = 1;
            calibed = false;
            prf.calMax[nowCal] = 0; 
            if (analogLed) ledcWrite(nowCal, 2);
            else b.setPixelColor(nowCal, b.Color(255, 255, 0));
          } 
          else if (calStep == 1 && !calibed) {
            if (prf.calMax[nowCal] - prf.calMin[nowCal] >= 100) {
              calibed = true;
              calStep = 2;
              if (analogLed) ledcWrite(nowCal, 4);
              else b.setPixelColor(nowCal, b.Color(0, 255, 0));
            } else {
              calibed = false;
              if (analogLed) ledcWrite(nowCal, 2);
              else b.setPixelColor(nowCal, b.Color(255, 255, 0));
            }
          }
        } else {
          if (calibed) {
            calibed = false;
            if (analogLed) ledcWrite(nowCal, 2);
            else b.setPixelColor(nowCal, b.Color(255, 255, 0));
          }
        }
        idk = millis();
        calLast1s = rawVal[nowCal];
      }
    }
    u8g2.clearBuffer();
    globFont();
    u8g2.drawStr(0, 12, ((calib ? "Calibrating S" : "Viewing S") + String(nowCal + 1)).c_str());
    u8g2.drawStr(0, 24, ("+ " + String((int)prf.calMax[nowCal])).c_str());
    u8g2.drawStr(0, 35, ("p " + String((int)rawVal[nowCal])).c_str());
    u8g2.drawStr(0, 46, ("- " + String((int)prf.calMin[nowCal])).c_str());
    u8g2.setFont(u8g2_font_5x8_tr);
    if (calib) {
      if (calStep == 0)      u8g2.drawStr( 93, 12, "RELEASE");
      else if (calStep == 1) u8g2.drawStr(103, 12, "PRESS");
      else if (calStep == 2) u8g2.drawStr(108, 12, "DONE");
    } else {
      u8g2.drawStr(108, 12, String(hallVal[nowCal]).c_str());
    }

    u8g2.drawStr(0, 56, "F1: Last"); u8g2.drawStr(50, 56, calib ? "F2: Stop Calib" : "F2: Calibrate");
    u8g2.drawStr(0, 64, "F3: Next"); u8g2.drawStr(50, 64, "F4: Exit");
    
    pushGraphValue(hallVal[nowCal]);
    drawGraph(40, 14);
    if (calib)
      u8g2.drawLine(40, (int)((1.0 - hallVal[nowCal]) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - hallVal[nowCal]) * (GRAPH_HEIGHT - 1)) + 14);
    else
      switch (prf.inputHandler) {
        case 0: u8g2.drawLine(40, (int)((1.0 - prf.actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.actuation) * (GRAPH_HEIGHT - 1)) + 14); break;
        case 1:
          u8g2.drawLine(40, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        case 2:
          u8g2.drawLine(40, (int)((1.0 - windowFoot[nowCal]) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - windowFoot[nowCal]) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - (windowFoot[nowCal] + prf.windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (windowFoot[nowCal] + prf.windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        default: break;
      }
    u8g2.sendBuffer();
    
    if (!analogLed) b.show();
  }

  for (int i = 0; i < 3; i++) {
    if (analogLed) ledcWrite(i, 0);
    else b.setPixelColor(i, b.Color(0, 0, 0));
  }
  if (!analogLed) b.show();
}

void inputMenu() {
  bool running = true;
  int hysteresisChange = 0;
  bool hysteresisChanging = false;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    graphData[i] = 0;
  }
  while (running) {
    if (firstTime) updateInput();
    u8g2.clearBuffer();
    float maxPress = max(max(hallVal[0], hallVal[1]), hallVal[2]);
    float maxFoot = max(max(windowFoot[0], windowFoot[1]), windowFoot[2]);
    pushGraphValue(maxPress);
    drawGraph(40, 14);
    globFont();
    switch (prf.inputHandler) {
      case 0:
      inputTypeDigitalEmulation();
        u8g2.drawStr(0, 12, "DigitalEmulation");
        u8g2.drawStr(0, 24, "Acc:");
        u8g2.drawStr(0, 36, (prf.hallDisplayAsKT ? String(prf.actuation * prf.keyTravel, 2) + "mm" : String(prf.actuation)).c_str());
        u8g2.drawLine(40, (int)((1.0 - prf.actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.actuation) * (GRAPH_HEIGHT - 1)) + 14);
        break;
      case 1:
        inputTypeHysteresisHandling();
        u8g2.drawStr(0, 12, "Hysteresis");
        u8g2.drawStr(0, 24, ("U: " + String(prf.upperThreshold * (prf.hallDisplayAsKT ? prf.keyTravel : 1))).c_str());
        u8g2.drawStr(0, 36, ("L: " + String(prf.lowerThreshold * (prf.hallDisplayAsKT ? prf.keyTravel : 1))).c_str());
        if (prf.hallDisplayAsKT) u8g2.drawStr(0, 48, "mm");
        u8g2.drawLine(40, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        u8g2.drawLine(40, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        if (hysteresisChange == 0) {
          u8g2.drawTriangle(40, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 10, 40, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 18, 46, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        } else {
          u8g2.drawTriangle(40, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 10, 40, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 18, 46, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        }
        break;
      case 2:
        inputTypeDynamicActuation();
        u8g2.drawStr(0, 12, "DynamicAct");
        u8g2.drawStr(0, 24, ("W: " + String(prf.windowSize * (prf.hallDisplayAsKT ? prf.keyTravel : 1))).c_str());
        if (prf.hallDisplayAsKT) u8g2.drawStr(0, 36, "mm");
        u8g2.drawLine(40, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14, 128  , (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14);
        u8g2.drawLine(40, (int)((1.0 - (maxFoot + prf.windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (maxFoot + prf.windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
        break;
      default:
        inputTypeDigitalEmulation();
        break;
    }
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(108, 12, String(maxPress).c_str());
    switch (prf.inputHandler) {
      case 0:
        u8g2.drawStr(0, 56, "F1: Acc+    F2: Change M"); // not enough space
        u8g2.drawStr(0, 64, "F3: Acc-    F4: Exit");
        break;
      case 1:
        if (!hysteresisChanging) {
          u8g2.drawStr(0, 56, "F1: Next    F2: Change M");
          u8g2.drawStr(0, 64, "F3: Change  F4: Exit");
        }
        else if (hysteresisChange == 0) {
          u8g2.drawStr(0, 56, "F1: U+      F2: Done");
          u8g2.drawStr(0, 64, "F3: U-      F4: Exit");
        }
        else if (hysteresisChange == 1) {
          u8g2.drawStr(0, 56, "F1: L+      F2: Done");
          u8g2.drawStr(0, 64, "F3: L-      F4: Exit");
        }
        break;
      case 2:
        u8g2.drawStr(0, 56, "F1: Ws+     F2: Change M");
        u8g2.drawStr(0, 64, "F3: Ws-     F4: Exit");
        break;
    }
    switch (getButton()) {
      case 0:
        if (prf.inputHandler == 0) {
          prf.actuation += 0.05;
          prf.actuation = constrain(prf.actuation, 0.05, 0.95);
        } else if (prf.inputHandler == 1) {
          if (!hysteresisChanging) {
            hysteresisChange = (hysteresisChange + 1) % 2;
          }
          else if (hysteresisChange == 0) {
            prf.upperThreshold += 0.05;
            prf.upperThreshold = constrain(prf.upperThreshold, prf.lowerThreshold + 0.05, 0.95);
          }
          else if (hysteresisChange == 1) {
            prf.lowerThreshold += 0.05;
            prf.lowerThreshold = constrain(prf.lowerThreshold, 0.05, prf.upperThreshold - 0.05);
          }
        } else if (prf.inputHandler == 2) {
          prf.windowSize += 0.05;
          prf.windowSize = constrain(prf.windowSize, 0.05, 1.00);
        }
        break;
      case 1:
        if (!(hysteresisChanging && prf.inputHandler == 1))
          prf.inputHandler = (prf.inputHandler + 1) % 3;
        else
          hysteresisChanging = false;
        break;
      case 2:
        if (prf.inputHandler == 0) {
          prf.actuation -= 0.05;
          prf.actuation = constrain(prf.actuation, 0.05, 0.95);
        } else if (prf.inputHandler == 1) {
          if (!hysteresisChanging) {
            hysteresisChanging = true;
          }
          else if (hysteresisChange == 0) {
            prf.upperThreshold -= 0.05;
            prf.upperThreshold = constrain(prf.upperThreshold, prf.lowerThreshold + 0.05, 0.95);
          }
          else if (hysteresisChange == 1) {
            prf.lowerThreshold -= 0.05;
            prf.lowerThreshold = constrain(prf.lowerThreshold, 0.05, prf.upperThreshold - 0.05);
          }
        } else if (prf.inputHandler == 2) {
          prf.windowSize -= 0.05;
          prf.windowSize = constrain(prf.windowSize, 0.05, 1.00);
        }
        break;
      case 3:
        running = false;
      default:
        break;
    }
    u8g2.sendBuffer();
  }
}

void changeFiltSet() {
  int sel = 1;
  while (sel > 0) {
    String opts = 
      "Filter: " + (String)(prf.doFilter ? "[On]" : "[Off]") + "\n" +
      (String)(prf.filterType == 0 ? "> OVS <": "OVS") + "\n" + 
      (String)(prf.filterType == 1 ? "> EMA <": "EMA") + "\n";
    
    if (prf.filterType == 0) opts += "OVS Samples: " + String(prf.ovsSamples);
    else opts += "EMA Alpha: " + String(prf.emaAlpha, 2);
    
    sel = noidMenu("Filter Settings", sel, opts.c_str());
    switch (sel) {
      case 1: prf.doFilter = !prf.doFilter; break;    
      case 2: prf.filterType = 0; break;
      case 3: prf.filterType = 1; break;
      case 4: {
        if (prf.filterType == 0) prf.ovsSamples = (int)valueSet("Samples (OVS)", prf.ovsSamples, true, 2, 256);
        else {
          prf.emaAlpha = valueSet("Alpha (EMA 1%)", prf.emaAlpha*100, true, 0.01, 100) / 100;
          prf.emaAlpha = constrain(prf.emaAlpha, 0.00001, 1);
        }
        break;
      }
      default: break;
    }
  }
}

void testFilt() {
  uint32_t oldLIU = LOOP_INTERVAL_US;
  globFont();

  auto getMaxHall = []() -> float {
    return max(max(hallVal[0], hallVal[1]), hallVal[2]);
  };
  float maxHall = getMaxHall();
  if (maxHall > 0.0f) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Please release all");
    u8g2.drawStr(0, 24, "Hall key!");
    u8g2.sendBuffer();

    while (getMaxHall() > 0.0f) {
      delay(10);
    }
  }
  const uint32_t testFreq[] = {1000, 500, 250, 125};
  unsigned long rels[4] = {0};
  float hrels = {0.00f};
  for (int i = 0; i < 4; i++) {
    LOOP_INTERVAL_US = testFreq[i];

    u8g2.clearBuffer();
    u8g2.drawStr(
      0,
      12,
      ("Now: " + String(1000000UL / testFreq[i]) + "Hz").c_str()
    );
    u8g2.drawStr(0, 24, "[Press]");
    u8g2.sendBuffer();
    while (getMaxHall() <= 0.0f) {
      delay(1);
    }
    unsigned long startTime = micros();
    bool timeout = false;
    const unsigned long TIMEOUT_US = 5000000UL; // 5s
    while (getMaxHall() < 1.0f) {

      if (micros() - startTime > TIMEOUT_US) {
        timeout = true;
        break;
      }
      delay(1);
    }
    rels[i] = timeout ? 0 : micros() - startTime;
    u8g2.clearBuffer();
    if (timeout) {
      u8g2.drawStr(0, 36, "Timeout!");
    } else {
      u8g2.drawStr(
        0,
        36,
        (String((float)rels[i] / 1000.0f, 2) + "ms").c_str()
      );
    }
    u8g2.drawStr(0, 48, "[Release]");
    u8g2.sendBuffer();

    while (getMaxHall() > 0.0f) {
      delay(10);
    }
  }
  LOOP_INTERVAL_US = oldLIU;
  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "Result:");
  for (int i = 0; i < 4; i++) {
    String line;
    if (rels[i] == 0) {
      line = String(1000UL / testFreq[i]) +
             "kHz: Timeout";
    } else {
      float actTime = 0;
      switch (prf.inputHandler) {
        case 0: actTime = (float)rels[i] / 1000.0f * prf.actuation; break;
        case 1: actTime = (float)rels[i] / 1000.0f * prf.upperThreshold; break;
        case 2: actTime = (float)rels[i] / 1000.0f * prf.windowSize; break;
        default: break;
      }
      line = String(1000UL / testFreq[i]) +
             "kHz: " +
             String((float)rels[i] / 1000.0f, 0) +
             "ms, | " +
             String(actTime, 2) + "ms";
    }
    u8g2.drawStr(
      0,
      12 * (i + 2),
      line.c_str()
    );
  }
  u8g2.sendBuffer();
  while (getButton() != 3) {
    delay(10);
  }
}

void filtMenu() {
  bool running = true;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    graphData[i] = 0;
  }
  while (running) {
    if (firstTime) updateInput();
    float maxHall = max(max(hallVal[0], hallVal[1]), hallVal[2]);
    float maxFoot = max(max(windowFoot[0], windowFoot[1]), windowFoot[2]);
    int btn = getButton();
    globFont();
    switch (btn) {
      case 0: prf.doFilter = !prf.doFilter; break;
      case 1: changeFiltSet(); break;
      case 2: testFilt(); break;
      case 3: running = false; break;
      default: break;
    }
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, prf.doFilter ? (prf.filterType == 0 ? "Filter [OVS]" : "Filter [EMA]") : "Filter [Off]");
    u8g2.drawStr(0, 24, ("1 " + String((int)rawVal[0])).c_str());
    u8g2.drawStr(0, 35, ("2 " + String((int)rawVal[1])).c_str());
    u8g2.drawStr(0, 46, ("3 " + String((int)rawVal[2])).c_str());
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(128 - u8g2.getStrWidth((String(lastRate) + "Hz").c_str()), 12, (String(lastRate) + "Hz").c_str());
    u8g2.drawStr(0, 56, prf.doFilter ? "F1: Disable" : "F1: Enable"); u8g2.drawStr(58, 56, "F2: Change");
    u8g2.drawStr(0, 64, "F3: Test"); u8g2.drawStr(58, 64, "F4: Exit");
    pushGraphValue(maxHall);
    drawGraph(40, 14);
      switch (prf.inputHandler) {
        case 0: u8g2.drawLine(40, (int)((1.0 - prf.actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.actuation) * (GRAPH_HEIGHT - 1)) + 14); break;
        case 1:
          u8g2.drawLine(40, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - prf.lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        case 2:
          u8g2.drawLine(40, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - (maxFoot + prf.windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (maxFoot + prf.windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        default: break;
      }
    u8g2.sendBuffer();
  }
}

void effectMenu() {
  int sel = 1;
  while (sel != 0) {
    globFont();
    String menu_items =
      "Under Glow " + String(prf.underGlow ? "[On]" : "[Off]") + "\n"
      + "RGB Led " + String(prf.rgb ? "[On]" : "[Off]");
    sel = noidMenu("Effects", sel, menu_items.c_str());
    if (sel == 1) {
      int subSel = 1;
      String effectsList[] = {
        "Tap",
        "Ripple",
        "Smooth",
        "Burn-in",
        "Analog",
        "Soild"
      };
      while (subSel != 0) {
        String tmp =
        "Under Glow: " + String(prf.underGlow ? "[On]" : "[Off]") + "\n"
        + "Type: " + effectsList[prf.glowType] + "\n";
        for (int i = 0; i < 6; i++) {
          tmp += (prf.glowType == i ? "> " : "") + effectsList[i] + (prf.glowType == i ? " <" : "");
          if (i < 5) tmp += "\n";
        }
        subSel = noidMenu("Under Glow", subSel, tmp.c_str(), true);
        if (subSel == 1) prf.underGlow = !prf.underGlow;
        else if (subSel >= 3 && subSel <= 8) prf.glowType = subSel - 3;
      }
    } else if (sel == 2) {
      int subSel = 1;
      while (subSel != 0){
        String menuElements =
          "RGB Led: " + String(prf.rgb ? "[On]" : "[Off]") + "\n"
          + "Brightness: " + String(prf.rgbBri) + "\n"
          + "Mode: " + String(prf.doRainbow ? "Rainbow" : "Static") + "\n";
        if (prf.doRainbow) {
          menuElements += "Speed: " + String(prf.rainbowStep) + "\n";
          menuElements += "Interval: " + String(prf.rgbInterval * 10) + " ms";
        } else {
          menuElements += "Color (" + String(prf.color[0]) + ", " + String(prf.color[1]) + ", " + String(prf.color[2]) + ")";
        }
        subSel = noidMenu("RGB Led", subSel, menuElements.c_str(), true);
        if (subSel == 1) prf.rgb = !prf.rgb;
        if (subSel == 2) prf.rgbBri = (uint8_t)valueSet("Brightness", prf.rgbBri, true, 0, 255);
        // u8g2.user.InterfaceInputValue("Brightness\n", "(0 - 255): ", &prf.rgbBri, 0, 255, 3, " ");
        if (subSel == 3) prf.doRainbow = !prf.doRainbow;
        if (subSel == 4) {
          if (prf.doRainbow) {
            //u8g2.user.InterfaceInputValue("Speed\n", "(1 - 255): ", &prf.rainbowStep, 1, 255, 3, " ");
            prf.rainbowStep = (uint8_t)valueSet("Speed", prf.rainbowStep, true, 1, 255);
          } else {
            int sub2Sel = 1;
            while (sub2Sel != 0) {
              String colorElements =
                + "R: " + String(prf.color[0]) + "\n"
                + "G: " + String(prf.color[1]) + "\n"
                + "B: " + String(prf.color[2]);
              sub2Sel = noidMenu("Color\n", sub2Sel, colorElements.c_str());
              if (sub2Sel == 1) prf.color[0] = (uint8_t)valueSet("Red", prf.color[0], true, 0, 255);
              else if (sub2Sel == 2) prf.color[1] = (uint8_t)valueSet("Blue", prf.color[1], true, 0, 255);
              else if (sub2Sel == 3) prf.color[2] = (uint8_t)valueSet("Green", prf.color[2], true, 0, 255);
            }
          }
        }
        if (subSel == 5) prf.rgbInterval = (uint8_t)valueSet("Interval", prf.rgbInterval, true, 1, 255);
      }
    }
  }
}

void displaySetting() {
  int subSel = 1;
  const char lazyAss[] =
    "1s\n"
    "5s\n"
    "10s\n"
    "20s\n"
    "30s\n"
    "1m\n"
    "5m\n"
    "10m"
  ;
  while (subSel != 0) {
    const String kaoOrSomethingIdk[] = {
      "Mufuki",
      "shy smile",
      "happy wide smile",
      "shock surprise",
      "angry frown",
      "crying shy",
      "shrug",
      "annoyed",
      "bashful",
      "excited grin",
      "lenny face",
      "Custom"
    };

    String menuItem =
    "Brightness: " + String(prf.screenBri) + "\n"
    + "Screen on: " + String(prf.screenSaveDuration / 1000) + "s\n"
    + "Screen save: " + String((prf.screenOffDuration) / 1000) + "s\n";
    if (prf.logoType == 0 || prf.logoType == 12)
      menuItem += "Icon: \"" + String(prf.screenLogo) + "\"";
    else
      menuItem += "Icon: \"" + String(kaoOrSomethingIdk[prf.logoType - 1]) + "\"";
    menuItem += "\nHall Value: " + (String)(prf.hallDisplayAsKT ? "mm" : "Normalized");
    menuItem += "\nKey travel: " + String(prf.keyTravel, 2) + "mm";
    menuItem += "\nMain Screen";
    menuItem += "\nAlways On Display";
    subSel = noidMenu("Display", subSel, menuItem.c_str());
    if (subSel == 1) {prf.screenBri = (uint8_t)valueSet("Brightness", prf.screenBri, true, 0, 255); u8g2.setContrast(prf.screenBri);}
    if (subSel == 2) {
      switch (noidMenu("Screen on", 1, lazyAss)) {
        case 1: prf.screenSaveDuration = 1000; prf.screenOffDuration += 1000; break;
        case 2: prf.screenSaveDuration = 5000; break;
        case 3: prf.screenSaveDuration = 10000; break;
        case 4: prf.screenSaveDuration = 20000; break;
        case 5: prf.screenSaveDuration = 30000; break;
        case 6: prf.screenSaveDuration = 60000; break;
        case 7: prf.screenSaveDuration = 300000; break;
        case 8: prf.screenSaveDuration = 600000; break;
        default: break;
      }
    }
    if (subSel == 3) {
      switch (noidMenu("Screen on", 1, lazyAss)) {
        case 1: prf.screenOffDuration = 1000; break;
        case 2: prf.screenOffDuration = 5000; break;
        case 3: prf.screenOffDuration = 10000; break;
        case 4: prf.screenOffDuration = 20000; break;
        case 5: prf.screenOffDuration = 30000; break;
        case 6: prf.screenOffDuration = 60000; break;
        case 7: prf.screenOffDuration = 300000; break;
        case 8: prf.screenOffDuration = 600000; break;
        default: break;
      }
    }
    if (subSel == 4) {
      String tmp = "";
      for (int i = 0; i < 12; i++) {
        tmp += kaoOrSomethingIdk[i];
        if (i < 11) tmp += "\n";
      }
      int logSel = noidMenu("Display Icon", 1, tmp.c_str());
      if (logSel == 1) {
        strcpy(prf.screenLogo, "Mufuki");
        prf.logoType = 0;
      } else if (logSel > 1 && logSel < 12) prf.logoType = logSel;
      else if (logSel != 0) {
        strcpy(prf.screenLogo, keyboard(prf.screenLogo).c_str());
        prf.logoType = 12;
      }
    }
    if (subSel == 5) prf.hallDisplayAsKT = !prf.hallDisplayAsKT;
    if (subSel == 6) {
      prf.keyTravel = valueSet("Key Travel (mm):", prf.keyTravel, true, 0.1, 1000);
      if (prf.keyTravel > 100)  u8g2.userInterfaceMessage("Damn", "da loooong way", "", " ok ");
    }
    if (subSel == 7) {
      int animSel = 1;
      while (animSel > 0) {
        bool isPlaying = visIsPlaying();
        String tmp = "Mode: " + (String)(visIsPlaying() ? "Animation" : "Normal") + "\n"
        + "Animation File";
        animSel = noidMenu("Main Screen", 1, tmp.c_str());
        if (animSel == 1) {
          if (isPlaying) visStop();
          else visPlay();
        } else if (animSel == 2) {
          std::vector<String> animations = listAnimations();
          if (animations.size()) {
            String animList = "";
            for (int i = 0; i < animations.size(); i++) {
              animList += animations[i];
              if (i < animations.size() - 1) animList += "\n";
            }
            int alaunch = noidMenu("Animation Files", 1, animList.c_str());
            if (alaunch > 0 && alaunch <= animations.size()) {
              visStop();
              visLoad(("/" + animations[alaunch - 1]).c_str());
            }
          } else {
            if (u8g2.userInterfaceMessage("No Animation Found", "Do you want to", "download demo?", " Yes \n No ") == 1) gitDownload();
          }
        }
      }
    }
    if (subSel == 8) {
      int aodSel = 1;
      while (aodSel > 0) {
        String tmp = "AOD: " + (String)(prf.AOD ? "On" : "Off") + "\n"
                   + "GMT Offset: " + String(prf.GMTPlus);
        aodSel = noidMenu("Always On Display", 1, tmp.c_str());
        if (aodSel == 1) {
          prf.AOD = !prf.AOD;
        } else if (aodSel == 2) {
          prf.GMTPlus = valueSet("GMT Offset:", prf.GMTPlus, true, 0, 12);
        }
      }
    }
  }
}

void mpuMenu() {
  const char useless[] =
    "Calibrate\n"
    "Inclinometer\n"
    "Speedometer"
  ;
  int sel = 1;
  while (sel != 0) {
    sel = noidMenu("MPU", sel, useless);
    if (sel == 1) {
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Calibrating") / 2, 32, "Calibrating...");
      u8g2.sendBuffer();
      mpu.calcOffsets();
      delay(1000);
    }
    if (sel == 2) {
      bool running = true;
      while (running) {
        mpu.update();
        int dx = mpu.getAngleY();
        int dy = -mpu.getAngleX();
        int px = map(constrain(dx, -70, 70), -70, 70, 32, 96);
        int py = map(constrain(dy, -70, 70), -70, 70, 0, 64);
        u8g2.clearBuffer();
        u8g2.drawLine(64, 0, 64, 64);
        u8g2.drawLine(0, 32, 128,  32);
        u8g2.drawCircle(px, py, 4);
        u8g2.sendBuffer();
        if (getButton() == 3) running = false;
      }
    }
    if (sel == 3) {
      float velX = 0, velY = 0, velZ = 0;
      float ax_f = 0, ay_f = 0, az_f = 0;
      const float ALPHA = 0.4;
      unsigned long lastTime = micros();
      unsigned long ldu = millis();
      bool kph = false;
      bool running = true;
      while (running) {
        mpu.update();

        unsigned long now = micros();
        float dt = (now - lastTime) / 1000000.0f; // s
        if (dt <= 0) return;
        lastTime = now;

        // raw accel in g
        float ax = mpu.getAccX();
        float ay = mpu.getAccY();
        float az = mpu.getAccZ();

        // simple low-pass to reduce high-frequency noise (helps angle calc)
        ax_f = ALPHA * ax + (1.0 - ALPHA) * ax_f;
        ay_f = ALPHA * ay + (1.0 - ALPHA) * ay_f;
        az_f = ALPHA * az + (1.0 - ALPHA) * az_f;

        // compute roll & pitch from accel (radians)
        float roll  = atan2(ay_f, az_f);
        float pitch = atan2(-ax_f, sqrt(ay_f * ay_f + az_f * az_f));

        const float G = 9.80665f;
        // gravity components in m/s^2 (body frame)
        float gX = -sin(pitch) * G;
        float gY =  sin(roll)  * cos(pitch) * G;
        float gZ =  cos(roll)  * cos(pitch) * G;

        // measured accel in m/s^2
        float accX = ax * G;
        float accY = ay * G;
        float accZ = az * G;

        // linear acceleration (remove gravity)
        float linAccX = accX - gX;
        float linAccY = accY - gY;
        float linAccZ = accZ - gZ;

        // small deadband to ignore sensor noise
        const float DEAD = 0.05f;
        if (fabs(linAccX) < DEAD) linAccX = 0;
        if (fabs(linAccY) < DEAD) linAccY = 0;
        if (fabs(linAccZ) < DEAD) linAccZ = 0;

        // integrate -> velocity
        velX += linAccX * dt;
        velY += linAccY * dt;
        velZ += linAccZ * dt;

        // time-independent damping (exponential) to avoid dependence on sample rate
        // choose tau (s) = 10s -> factor = exp(-dt/tau)
        const float tau = 10.0f;
        float damp = expf(-dt / tau);
        velX *= damp;
        velY *= damp;
        velZ *= damp;

        float speed = sqrtf(velX*velX + velY*velY + velZ*velZ);

        if (millis() - ldu > 200) {
          u8g2.clearBuffer();
          String tmp = String(int(speed * (kph ? 3.6 : 1))) + (kph ? "km/h" : "m/s");
          u8g2.setFont(u8g2_font_spleen16x32_mr);
          u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 40, tmp.c_str());
          globFont();
          tmp = "X: " + String(int(velX)) + "m/s | Y: " + String(int(velY)) + "m/s | Z: " + String(int(velZ)) + "m/s";
          u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 54, tmp.c_str());
          u8g2.sendBuffer();
          ldu = millis();
        }
        int btn = getButton();
        if (btn == 0) kph = !kph;
        if (btn == 1) {
          velX = 0.00;
          velY = 0.00;
          velZ = 0.00;
        }
        if (btn == 3) running = false;
        delay(12);
      }
    }
  }
}

void showDebug() {
  bool running = true;
  int page = 0;
  const int pages = 5;
  u8g2.setFont(u8g2_font_5x8_tr);
  while (running) {
    u8g2.clearBuffer();
    // updateInput();
    int btn = getButton();
    if (btn == 0) page = (page + (pages - 1)) % pages;
    if (btn == 1) page = (page + 1) % pages;
    if (btn == 3) running = false;
    switch (page) {
      case 0: {
        for (int i = 0; i < 3; i++) {
          String tmp = "S" + String(i + 1) + ":" + String(hallVal[i]);
          u8g2.drawStr(0, 10 + i * 10, tmp.c_str());
          tmp = "Raw" + String(i + 1) + ":" + String(rawVal[i]);
          u8g2.drawStr(78, 10 + i * 10, tmp.c_str());
          tmp = "Min" + String(i + 1) + ":" + String(int(prf.calMin[i]));
          u8g2.drawStr(0, 40 + i * 10, tmp.c_str());
          tmp = "Max" + String(i + 1) + ":" + String(int(prf.calMax[i]));
          u8g2.drawStr(78, 40 + i * 10, tmp.c_str());
        }
        break;
      }
      case 1: {
        String tmp = "chip:" + String(ESP.getChipModel());
        u8g2.drawStr(0, 10, tmp.c_str());
        tmp = "cores:" + String(ESP.getChipCores());
        u8g2.drawStr(0, 20, tmp.c_str());
        tmp = "free:" + String(ESP.getFreeHeap()) + "b";
        u8g2.drawStr(0, 30, tmp.c_str());
        tmp = "heap:" + String(ESP.getHeapSize()) + "b";
        u8g2.drawStr(0, 40, tmp.c_str());
        tmp = "flash:" + String(ESP.getFlashChipSize()) + "b";
        u8g2.drawStr(0, 50, tmp.c_str());
        tmp = "temp:" + String(temperatureRead()) + " deg";
        u8g2.drawStr(0, 60, tmp.c_str());
        break;
      }
      case 2: {
        String tmp = "ip:" + WiFi.localIP().toString();
        u8g2.drawStr(0, 10, tmp.c_str());
        tmp = "ssid:" + String(WiFi.SSID());
        u8g2.drawStr(0, 20, tmp.c_str());
        tmp = "rssi:" + String(WiFi.RSSI());
        u8g2.drawStr(0, 30, tmp.c_str());
        tmp = "mac:" + String(WiFi.macAddress());
        u8g2.drawStr(0, 40, tmp.c_str());
        tmp = "adcDeadZone: [" + String(prf.deadZone[0]) + ", " + String(prf.deadZone[1]) + ", " + String(prf.deadZone[2]) + "]";
        u8g2.drawStr(0, 50, tmp.c_str());
        break;
      }
      case 3: {
        mpu.update();
        u8g2.drawStr(0, 10, "[XYZ]");
        u8g2.drawStr(0, 20, "acc:"); u8g2.drawStr(30, 20, String(mpu.getAccX()).c_str()); u8g2.drawStr(70, 20, String(mpu.getAccY()).c_str()); u8g2.drawStr(110, 20, String(mpu.getAccZ()).c_str());
        u8g2.drawStr(0, 30, "ang:"); u8g2.drawStr(30, 30, String(mpu.getAngleX()).c_str()); u8g2.drawStr(70, 30, String(mpu.getAngleY()).c_str()); u8g2.drawStr(110, 30, String(mpu.getAngleZ()).c_str());
        u8g2.drawStr(0, 40, "gyr:"); u8g2.drawStr(30, 40, String(mpu.getGyroX()).c_str()); u8g2.drawStr(70, 40, String(mpu.getGyroY()).c_str()); u8g2.drawStr(110, 40, String(mpu.getGyroZ()).c_str());
        u8g2.drawStr(0, 50, "tmp:"); u8g2.drawStr(30, 50, String(mpu.getTemp()).c_str());
        break;
      }
      case 4: { // Debug FS
        size_t total = LittleFS.totalBytes();
        size_t used  = LittleFS.usedBytes();
        String tmp = "FS total:" + String(total) + "b";
        u8g2.drawStr(0, 10, tmp.c_str());
        tmp = "FS used :" + String(used) + "b";
        u8g2.drawStr(0, 20, tmp.c_str());
        tmp = "FS free :" + String(total - used) + "b";
        u8g2.drawStr(0, 30, tmp.c_str());
        tmp = "FS %   :" + String((used * 100) / total) + "%";
        u8g2.drawStr(0, 40, tmp.c_str());
        break;
      }
      default: break;
    }
    u8g2.sendBuffer();
  }
}

void deadCalib() {
  const char menuItems[] =
    "Auto Calibrate\n"
    "Manual Set"
  ;
  int option = 1;
  while (option != 0) {
    option = noidMenu("DeadZone Calibrate", option, menuItems);
    if (option == 1) {
      for (int i = 0; i < 5; i++){
        u8g2.clearBuffer();
        u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
        u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
        String tmp = "<<< " + String(5 - i) + "s >>>";
        u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
        u8g2.sendBuffer();
        delay(1000);
      }
      u8g2.clearBuffer();
      u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
      u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
      String tmp = "[Started]";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
      u8g2.sendBuffer();
      int aMax = 0;
      int aMin = 4095;
      delay(1000);
      for (int sw = 0; sw < 3; sw++) {
        aMax = 0;
        aMin = 4095;
        u8g2.clearBuffer();
        u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
        u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
        String tmp = "[" + String(sw + 1) + "/3]";
        u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
        u8g2.sendBuffer();
        for (int i = 0; i < 2000; i++) {
          int adcVal = 0;
          if (prf.doFilter) {
            if (prf.filterType == 0) {
              adcVal = overSample(sw);
            } else {
              adcVal = expoMovAvr(sw);
            }
          } else adcVal = analogRead(adcPins[sw]);
          if (adcVal > aMax) aMax = adcVal;
          if (adcVal < aMin) aMin = adcVal;
          delay(1);
        }
        prf.deadZone[sw] = aMax - aMin + 4;
      }
      int avrgMin = (prf.calMin[0] + prf.calMin[1] + prf.calMin[2]) / 3;
      int avrgMax = (prf.calMax[0] + prf.calMax[1] + prf.calMax[2]) / 3;
      int avrgDZ = (prf.deadZone[0] + prf.deadZone[1] + prf.deadZone[2]) / 3;
      if (avrgMax - avrgMin - avrgDZ * 2 < 100) {
        int opt = u8g2.userInterfaceMessage(
          "Calibration failed!",
          "Dead zone too big",
          "Please try again",
          " Ok \n More Info "
        );
        if (opt == 2) {
          u8g2.userInterfaceMessage(
            "[1/2] More Info",
            ("Min: " + String(aMin)).c_str(),
            ("Max: " + String(aMax)).c_str(),
            " Next>> "
          );
          u8g2.userInterfaceMessage(
            "[2/2] More Info",
            ("DeadZone: " + String(avrgDZ)).c_str(),
            ("Dynamic Range: " + String(avrgMax - avrgMin - avrgDZ * 2)).c_str(),
            " Next>> "
          );
        }
      }
      String dzs = "Dead Zone: ";
      for (int i = 0; i < 3; i++) {
        dzs += String(prf.deadZone[i]) + (i < 2 ? "-" : "");
      }
      u8g2.userInterfaceMessage(
        "Calibration done!",
        dzs.c_str(),
        "",
        " Ok "
      );
    }
    else if (option == 2) {
      int dSel = 1;
      while (dSel > 0) {
        String dList = 
        "Hall 1: " + String(prf.deadZone[0])+
        "\nHall 2: " + String(prf.deadZone[1])+
        "\nHall 3: " + String(prf.deadZone[2]);
        dSel = noidMenu("Manual Set", dSel, dList.c_str());
        if (dSel > 0) prf.deadZone[dSel - 1] = (int)valueSet(("Dead Zone " + String(dSel)).c_str(), prf.deadZone[dSel - 1], true, 0, 4095);
      }
    }
  }
}

void waitAction(bool state) {
  bool waitK = state;
  while (getButton() > 0) delay(10);
  while (waitK) {
    waitK = getButton() == -1;
    delay(10);
  }
  while (getButton() > 0) delay(10);
}

void splScreen(const char* title, const char* t1, const char* t2, const char* btn, bool dobtn, bool waitKey) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_tenthinguys_tf);
  u8g2.drawStr(0, 12, title);
  globFont();
  u8g2.drawStr(0, 25, t1);
  u8g2.drawStr(0, 37, t2);
  if (dobtn)
    u8g2.drawButtonUTF8(120 - u8g2.getStrWidth(btn), 64 - u8g2.getMaxCharHeight(), U8G2_BTN_INV, 0,  2,  2, btn);
  u8g2.sendBuffer();
  waitAction(waitKey);
}

void firstTimeSetup() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_spleen16x32_mr);
  for (int i = 0; i < 41; i++) {
    u8g2.clearBuffer();
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, i, "Mufuki");
    u8g2.sendBuffer();
    //delay(8);
  }
  globFont();
  u8g2.drawStr((128 - u8g2.getStrWidth("NoID"))/2, 54, "NoID");
  u8g2.sendBuffer();
  delay(500);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 42, 128, 22);
  u8g2.setDrawColor(1);
  u8g2.drawStr((128 - u8g2.getStrWidth(("NoID - " + ver).c_str()))/2, 54, ("NoID - " + ver).c_str());
  u8g2.sendBuffer();
  delay(1000);
  splScreen("Wellcome!", "Mufuki Setup", (ver + " - NoID").c_str(),  " Start>> ");
  splScreen("Switches", "Calibrate?", "", "", false, false);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 56, "F1: OK");
  u8g2.drawStr(0, 64, "F2: Skip");
  u8g2.sendBuffer();
  int btn = -1;
  while (btn != 1) {
    btn = getButton();
    if (btn == 0) {
      calibMenu();
      btn = 1;
    }
    delay(10);
  }
  splScreen("Switches", "Use recommended", "settings and done?", "", false, false);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 56, "F1: OK");
  u8g2.drawStr(0, 64, "F2: Continue");
  u8g2.sendBuffer();
  btn = -1;
  while (btn != 1) {
    btn = getButton();
    if (btn == 0) {
      prf.doFilter = true;
      prf.filterType = 1;
      prf.actuation = 2;
      prf.windowSize = 0.25;
      prf.rgb = true;
      prf.color[0] = 255;
      prf.color[1] = 1;
      prf.color[2] = 224;
      saveProfile(configPath.c_str(), prf);
      sysSave();
      splScreen("Setup Done!", "You can hold F4", "to enter menu", " Finish ");
      return;
      btn = 1;
    }
    delay(10);
  }
  splScreen("Input Handler", "Now: Digital Emulation", "Actuation: 0.3", "", false, false);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 56, "F1: Change");
  u8g2.drawStr(0, 64, "F2: Skip");
  u8g2.sendBuffer();
  btn = -1;
  while (btn != 1) {
    btn = getButton();
    if (btn == 0) {
      inputMenu();
      btn = 1;
    }
    delay(10);
  }
  splScreen("Noise Filter", "Help stabilize input", "Now: Off", "", false, false);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 56, "F1: Change");
  u8g2.drawStr(0, 64, "F2: Skip");
  u8g2.sendBuffer();
  btn = -1;
  while (btn != 1) {
    btn = getButton();
    if (btn == 0) {
      filtMenu();
      btn = 1;
    }
    delay(10);
  }
  splScreen("Effects", "To show off ig?", "Now: All Off", "", false, false);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 56, "F1: Change");
  u8g2.drawStr(0, 64, "F2: Skip");
  u8g2.sendBuffer();
  btn = -1;
  while (btn != 1) {
    btn = getButton();
    if (btn == 0) {
      effectMenu();
      btn = 1;
    }
    delay(10);
  }
  saveProfile(configPath.c_str(), prf);
  sysSave();
  splScreen("Setup Complete!", "Your device are", "ready to use", " Next ");
  splScreen("Setup Done!", "You can hold F4", "to enter menu");
}

void fomartFS() {
  int opt = u8g2.userInterfaceMessage(
    "Warning!",
    "Clear all files and",
    "restart device?",
    " Yes \n No "
  );
  if (opt != 1) return;
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    u8g2.clearBuffer();
    u8g2.drawStr((128 - u8g2.getStrWidth("Error!"))/2, 28, "Error!");
    u8g2.drawStr((128 - u8g2.getStrWidth("Can't open FS"))/2, 40, "Can't open FS");
    u8g2.sendBuffer();
    delay(1000);
    return;
  }

  File file = root.openNextFile();
  u8g2.clearBuffer();
  globFont();
  u8g2.drawStr((128 - u8g2.getStrWidth("Clearing Flash..."))/2, 28, "Clearing Flash...");
  u8g2.sendBuffer();
  LittleFS.end();

  if (LittleFS.format()) {
    u8g2.clearBuffer();
    u8g2.drawStr((128 - u8g2.getStrWidth("Clear"))/2, 28, "Clear");
    delay(1000);
  }
  forceReset();
}

void otaUpdate() {
  if (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) {;
    while (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) {
      int c = u8g2.userInterfaceMessage(
      "No Connection",
      "Please connect",
      "to WiFi first",
      " WiFi \n Back "
      );
      if (c == 1) wifiMenu();
      else break;
    }
    if (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) return;
  }
  static uint8_t spinnerIndex = 0;
  static bool doingOTA = true;
  const char spinnerFrames[4] = {'|', '/', '-', '\\'};
  unsigned long displayTime = millis();
  l.setBrightness(128);

  ArduinoOTA
    .onProgress([&](unsigned int progress, unsigned int total) {
      int percent = (progress * 100) / total;

      u8g2.clearBuffer();
      globFont();

      if (percent == 0) {
        char frame[2] = {spinnerFrames[spinnerIndex], '\0'};
        spinnerIndex = (spinnerIndex + 1) % 4;

        u8g2.drawStr((128 - u8g2.getStrWidth("Waiting..."))/2, 20, "Waiting...");
        u8g2.drawStr((128 - u8g2.getStrWidth(frame))/2, 48, frame);
      } else {
        u8g2.drawStr((128 - u8g2.getStrWidth("Updating..."))/2, 14, "Updating...");
        u8g2.drawFrame(10, 28, 108, 12);
        int barWidth = (108 * percent) / 100;
        u8g2.drawBox(10, 28, barWidth, 12);

        String percentStr = String(percent) + "%";
        u8g2.drawStr((128 - u8g2.getStrWidth(percentStr.c_str()))/2, 54, percentStr.c_str());
      }
      if (millis() - displayTime > 1000) {
        l.fill(l.Color(255, 170, 0));
        l.show();
        u8g2.sendBuffer();
        displayTime = millis();
      }
    })
    .onEnd([]() {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_spleen16x32_mr);
      u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
      globFont();
      u8g2.drawStr((128 - u8g2.getStrWidth("Restarting..."))/2, 54, "Restarting...");
      u8g2.sendBuffer();
      l.fill(l.Color(255, 255, 255));
      l.show();
      forceReset();
      doingOTA = false;
    });

  ArduinoOTA.begin();

  u8g2.clearBuffer();
  globFont();
  u8g2.drawStr((128 - u8g2.getStrWidth("OTA Update"))/2, 20, "OTA Update");
  IPAddress ip = WiFi.localIP();
  String ipStr = ip.toString();

  ipStr = "IP: " + ipStr;
  u8g2.drawStr((128 - u8g2.getStrWidth(ipStr.c_str()))/2, 63, ipStr.c_str());
  u8g2.drawStr((128 - u8g2.getStrWidth("Waiting..."))/2, 36, "Waiting...");
  u8g2.sendBuffer();

  l.fill(l.Color(0, 255, 0));
  l.show();

  while (doingOTA) {
    ArduinoOTA.handle();
    int btn = getButton();
    if (btn == 3) {doingOTA = false; break;}
    delay(100);
  }
  forceReset();
}

void fileMan() {
  bool brw = true;
  String currentPath = "/";
  while (brw) {
    std::vector<String> files = listFiles(currentPath.c_str());
    String fileList = "";
    for (int i = 0; i < files.size(); i++) {
      fileList += files[i];
      if (i < files.size() - 1) fileList += "\n";
    }
    int sel = noidMenu(currentPath.c_str(), 1, fileList.c_str());
    if (sel > 0 && sel <= files.size()) {
      String selectedFile = files[sel - 1];
      if (selectedFile.endsWith("/")) {
        currentPath += selectedFile;
      } else {
        int action = u8g2.userInterfaceMessage(
          selectedFile.c_str(),
          "What do you want to do?",
          "",
          " Delete \n Back "
        );
        if (action == 1) {
          LittleFS.remove((currentPath + selectedFile).c_str());
        }
      }
    } else {
      brw = false;
    }
  }
}

void systemMenu() {
  int sel = 1;
  bool ls = morseKey;
  const char menuItems[] =
    "Display\n"
    "MPU\n"
    "Reset\n"
    "Debug\n"
    "File Manager\n"
    "OTA Update\n"
    "Keyboard Type"
  ;
  while (sel != 0) {
    globFont();
    sel = noidMenu("System", sel, menuItems);
    switch (sel) {
      case 1: displaySetting(); break;
      case 2: mpuMenu(); break;
      case 3: fomartFS(); break;
      case 4: showDebug(); break;
      case 5: fileMan(); break;
      case 6: otaUpdate(); break;
      case 7: {
        int opt = noidMenu("Keyboard Type", 0, morseKey ? "Qwerty\n> Morse <" : "> Qwerty <\nMorse");
        if (opt == 2) ls = true;
        else if (opt == 1) ls = false;
        break;
      }
      default: break;
    }
  }
  if (ls != morseKey) {
    int opt = u8g2.userInterfaceMessage("Save keyboard", "type to system?", "", " Yes \n No ");
    if (opt == 1) sysSave();
  }
}

void about() {
  bool running = true;
  int estCount = 0;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim_11_idk);
  String tmp = "Version: " + ver;
  u8g2.drawStr(64 - u8g2.getStrWidth(tmp.c_str()) / 2, 27, tmp.c_str());
  u8g2.drawStr(64 - u8g2.getStrWidth("By NoID Team") / 2, 39, "By NoID Team");
  u8g2.sendBuffer();
  while (running) {
    switch (getButton()) {
      case 0: estCount++; break;
      case 1: if (estCount > 5) running = false; break;
      case 3: return;
      default: break;
    }
    delay(10);
  }
  
  l.setBrightness(255);
  l.fill(l.Color(255, 0, 255));
  l.show();
  // if you are a programmer or just want to show off
  // feel free to add your name here, just make sure it is less than 32 characters
  const int numProgrammers = 1;
  const char programmers[numProgrammers][32] = {
    "Khang238" // <--- the one who made everything btw
  };

  unsigned long lastUpdate = 0;
  for (int i = 0; i < numProgrammers; i++) {
    lastUpdate = millis();
    while (millis() - lastUpdate < 2000) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.drawStr(0, 8, "Made with love by:");
      if (strlen(programmers[i]) < 9) u8g2.setFont(u8g2_font_spleen16x32_mr);
      else u8g2.setFont(u8g2_font_gulim_11_idk);
      drawScrambleText(0, 32, programmers[i]);
      u8g2.sendBuffer();
    }
  }
  lastUpdate = millis();
  while (millis() - lastUpdate < 2000) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(0, 8, "Made with love by:");
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    drawScrambleText(0, 32, "NoID");
    u8g2.sendBuffer();
  }
  lastUpdate = millis();
  while (millis() - lastUpdate < 2000) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    drawScrambleText(0, 32, "---------");
    u8g2.sendBuffer();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim_11_idk);
  const char* ch1 = "Thank you";
  const char* ch2 = "For choosing Mufuki";
  const char* ch3 = "as your companion.";
  u8g2.drawStr(64 - u8g2.getStrWidth(ch1) / 2, 18, ch1);
  u8g2.drawStr(64 - u8g2.getStrWidth(ch2) / 2, 32, ch2);
  u8g2.drawStr(64 - u8g2.getStrWidth(ch3) / 2, 46, ch3);
  u8g2.setFont(u8g2_font_5x8_tr);
  tmp = ver + " - NoID 2026";
  u8g2.drawStr(128 - u8g2.getStrWidth(tmp.c_str()), 64, tmp.c_str());
  u8g2.sendBuffer();
  while (getButton() == -1) delay(10);
  lastUpdate = millis();
  while (millis() - lastUpdate < 1000) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    drawScrambleText(0, 32, "Love You!");
    u8g2.sendBuffer();
  }
  u8g2.clearBuffer();
  u8g2.drawXBMP(20, 20, 88, 20, logoKao[0]);
  u8g2.sendBuffer();
  delay(1000);
  l.fill(l.Color(0, 0, 0));
  l.show();
}

void wifiConnectScreen() {
  unsigned long beginTime = millis();
  while (millis() - beginTime < 60000 && WiFi.status() != WL_CONNECTED) {
    int btnPress = getButton();
    if (btnPress == 3) beginTime = millis() - 62000;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 34, "Mufuki");
    globFont();
    String conDot = "Connecting";
    for (int i = 0; i < (int)(millis() / 500 % 4); i++) conDot += ".";
    u8g2.drawStr((128 - u8g2.getStrWidth(conDot.c_str()))/2, 48, conDot.c_str());
    if (millis() - beginTime > 5000) {
      u8g2.setFont(u8g2_font_5x8_tr);
      String tmp = "C" + String((int)WiFi.status()) + ", " + String((millis() - beginTime) / 1000) + "s/60s, F4 to abort";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 62, tmp.c_str());
    }
    u8g2.sendBuffer();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_spleen16x32_mr);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 34, "Mufuki");
  globFont();
  switch (WiFi.status()) {
    case WL_CONNECTED:
      u8g2.drawStr((128 - u8g2.getStrWidth("Connected"))/2, 48, "Connected");
    break;
    default:
      u8g2.drawStr((128 - u8g2.getStrWidth("Not connected"))/2, 48, "Not connected");
    break;
  }
  u8g2.sendBuffer();
  delay(2000);
}

void wifiMenu() {
  globFont();
  int subSel = 1;
  int wifiCount = -1;
  while (subSel != 0) {
    String wItems = "";
    bool wStatus = (WiFi.status() == WL_CONNECTED);
    bool wDisable = !(WiFi.getMode() != WIFI_OFF);
    if (wDisable) wItems += "Enable WiFi: [OFF]";
    else if (wStatus) {
      wItems += "Enable WiFi: [ON]\n";
      wItems += "Status: [Connected]\n";
      wItems += "IP: " + WiFi.localIP().toString() + "\n";
      wItems += "SSID: " + WiFi.SSID() + "\n";
      wItems += "Strength: " + String(map(constrain(WiFi.RSSI(), -100, -10), -100, -10, 0, 100)) + "% (" + String(WiFi.RSSI()) + ")\n";
      wItems += "Disconnect";
    } else {
      wItems += "Enable WiFi: [ON]\n";
      wItems += "Status: [Offline]\n";
      wItems += "Scan for networks\n";
      wItems += "Manual Connect";
    }
    subSel = noidMenu("WiFi Settings", subSel, wItems.c_str());
    if (subSel == 1) {
      if (wDisable) {WiFi.mode(WIFI_STA); WiFi.begin();}
      else WiFi.mode(WIFI_OFF);
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Applying...") / 2, 32, "Applying...");
      u8g2.sendBuffer();
      delay(500);
    }
    if (subSel == 2) {
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Refreshing...") / 2, 32, "Refreshing...");
      u8g2.sendBuffer();
      delay(200);
    }
    if (subSel == 3) {
      if (!wStatus) { // do nothing with the wifi ip
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_spleen16x32_mr);
        u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
        globFont();
        u8g2.drawStr((128 - u8g2.getStrWidth("Searching..."))/2, 54, "Searching...");
        u8g2.sendBuffer();
        unsigned long scanStart = millis();
        wifiCount = WiFi.scanNetworks();
        if (millis() - scanStart < 1000) { // something went wrong with the scan so we have to reset the wifi (happens when the esp32 is in a weird state, not sure why)
          WiFi.mode(WIFI_OFF);
          delay(100);
          WiFi.mode(WIFI_STA);
          delay(100);
          unsigned long refreshStart = millis();
          wifiCount = WiFi.scanNetworks();
          if (millis() - refreshStart < 1000) wifiCount = 0; // if it still fails, just show no networks
        }
        if (wifiCount == 0) {
          u8g2.userInterfaceMessage("No Networks", "Found!", "", " Ok ");
        }
        else {
          int wsSel = 1;
          while (wsSel != 0) {
            String Wlist = "Rescan\n";
            for (int i = 0; i < wifiCount; i++) {
              Wlist += String(i + 1) + ". " + WiFi.SSID(i) + " (" + String(map(constrain(WiFi.RSSI(i), -100, -10), -100, -10, 0, 100)) + "%)" + (i < wifiCount - 1 ? "\n" : "");
            }
            wsSel = noidMenu("Select Network", wsSel, Wlist.c_str());
            if (wsSel == 1) {
              u8g2.clearBuffer();
              u8g2.setFont(u8g2_font_spleen16x32_mr);
              u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
              globFont();
              u8g2.drawStr((128 - u8g2.getStrWidth("Searching..."))/2, 54, "Searching...");
              u8g2.sendBuffer();
              wifiCount = WiFi.scanNetworks();
              if (wifiCount == 0) {
                u8g2.userInterfaceMessage("No Networks", "Found!", "", " Ok ");
                wsSel = 0;
              }
            } else if (wsSel > 1) {
              String wifiPass = keyboard("");
              WiFi.begin(WiFi.SSID(wsSel - 2), wifiPass);
              wifiConnectScreen();
              wsSel = 0;
            }
          }
        }
      }
    }
    if (subSel == 4) {
      if (!wStatus) {
        int wsSel = 1;
        String wifiSSID = "";
        String wifiPass = "";
        while (wsSel != 0) {
          String wConItems =
          "SSID: " + (wifiSSID == "" ? "<Required!>" : wifiSSID) + "\n"
        + "Pass: " + (wifiPass == "" ? "<None>" : wifiPass) + "\n"
        + (wifiSSID == "" ? "[type SSID!]" : "Connect!");
          wsSel = noidMenu("Manual connect", wsSel, wConItems.c_str());
          if (wsSel == 1) wifiSSID = keyboard(wifiSSID);
          if (wsSel == 2) wifiPass = keyboard(wifiPass);
          if (wsSel == 3 && wifiSSID != "") {
            WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
            wifiConnectScreen();
            wsSel = 0;
          }
        }
      }
    }
    if (subSel == 6) {
      WiFi.disconnect();
      u8g2.clearBuffer();
      u8g2.drawStr(64 - u8g2.getStrWidth("Disconnecting...") / 2, 32, "Disconnecting...");
      u8g2.sendBuffer();
      delay(500);
    }
  }
}

void layoutChangeMenu() {
  int layChange = 1;
  String layoutName[] = {
    "Standard",
    "WASD",
    "Arrows",
    "Shortcuts",
    "For BLE Key",
    "Custom"
  };
  while (layChange != 0) {
    String tmpName = "";
    for (int i = 0; i < 6; i++) {
      tmpName += (i == prf.layoutType ? "> " : "") + layoutName[i] + (i == prf.layoutType ? " <" : "");
      if (i < 5) tmpName += "\n";
    }
    layChange = noidMenu("Layout", prf.layoutType + 1, tmpName.c_str());
    if (layChange == 0) continue;
    prf.layoutType = layChange != 0 ? layChange - 1 : prf.layoutType;
    if (prf.layoutType == 5) {
      int layoutSel = 1;
      while (layoutSel != 0) {
        String nowLayout = "";
        for (int i = 0; i < 6; i++) {
          if (i < 3) nowLayout += "Sw" + String(i + 1) + ": ";
          else nowLayout += "F" + String(i - 2) + ": ";
          if (i >= 3 && prf.launchMacro[i - 3] > -1) nowLayout += "Macro " + String(prf.launchMacro[i - 3] + 1);
          else nowLayout += codeToName(prf.layout[i]);
          if (i < 5) nowLayout += "\n";
        }
        layoutSel = noidMenu("Custom Layout", layoutSel, nowLayout.c_str());
        if (layoutSel > 0) {
          bool lv = false;
          if (layoutSel > 3) {
            int sl = noidMenu("Change Function", 1, "Key\nMacro");
            if (sl == 2) {
              int sopt = noidMenu("Macro", 1, "Slot 1\nSlot 2\nSlot 3");
              if (sopt > 0) prf.launchMacro[layoutSel - 4] = sopt - 1;
              prf.layout[layoutSel - 1] = HID_KEY_NONE;
              lv = true;
            } else if (sl == 1) {prf.launchMacro[layoutSel - 4] = -1;}
            else lv = true;
          }
          if (!lv) {
            int ckeycode = noidMenu("Change Key", codeToIndex(prf.layout[layoutSel - 1]), buttonName);
            if (ckeycode > 0) prf.layout[layoutSel - 1] = buttonCode[ckeycode - 1];
          }
        }
      }
    } else {
      layChange = 0;
      for (int i = 0; i < 6; i++) {
        prf.layout[i] = preLayout[prf.layoutType][i];
      }
    }
  }
}

void connectMenu() {
  bool bModeC = withBLE;
  int sel = 1;
  int vpidChange = vpidSet;
  while (sel > 0) {
    String items =
      "USB HID\n"
      "Bluetooth: " + (String)(bModeC ? "Enabled" : "Disabled")
      + "\nWiFi Settings\n"
      "Mode: ";
    switch (usbMode) {
    case 0: items += "Keyboard"; break;
    case 1: items += "Gamepad"; break;
    case 2: items += "Mouse"; break;
    default: break;
    }
    sel = noidMenu("Connection", sel, items.c_str());
    if (sel == 1) {
      int subSel = 1;
      while (subSel != 0) {
        String usbItems =
          "Status: " + String(tud_ready() ? "[OK]" : "[Offline]") + "\n"
          "Always Report: " + String(alwaysReport ? "[On]" : "[Off]") + "\n"
          "Polling Rate: " + String(1000000 / LOOP_INTERVAL_US) + " Hz";
        if (usbMode == 1) {
          usbItems += "VID PID Set: ";
          switch (vpidChange) {
            case 0: usbItems += "PS4"; break;
            case 1: usbItems += "Xbox"; break;
            case 2: usbItems += "Switch"; break;
            case 3: usbItems += "Logitech"; break;
            default: break;
          }
        }
        subSel = noidMenu("USB HID", subSel, usbItems.c_str());
        if (subSel == 1) {
          u8g2.clearBuffer();
          u8g2.drawStr(64 - u8g2.getStrWidth("Refreshing...") / 2, 32, "Refreshing...");
          u8g2.sendBuffer();
          delay(200);
        }
        if (subSel == 2) alwaysReport = !alwaysReport;
        else if (subSel == 3) {
          const char pollingList[] =
            "125 Hz\n"
            "250 Hz\n"
            "500 Hz\n"
            "1000 Hz\n"
            "2000 Hz\n"
            "4000 Hz [!]\n"
            "Unlimited [!]";
          int pollSel = noidMenu("Polling Rate", 1, pollingList);
          switch (pollSel) {
            case 1: LOOP_INTERVAL_US = 8000; break;
            case 2: LOOP_INTERVAL_US = 4000; break;
            case 3: LOOP_INTERVAL_US = 2000; break;
            case 4: LOOP_INTERVAL_US = 1000; break;
            case 5: LOOP_INTERVAL_US = 500; break;
            case 6: {
              int confirm = u8g2.userInterfaceMessage(
                "Warning!",
                "4000Hz may unstable",
                "Continue?",
                " Yes \n No "
              );
              if (confirm == 1) LOOP_INTERVAL_US = 250;
              break;
            }
            case 7: {
              int confirm = u8g2.userInterfaceMessage(
                "Warning!",
                "Feature unstable",
                "Continue?",
                " Yes \n No "
              );
              if (confirm == 1) LOOP_INTERVAL_US = 1;
              break;
            }
            default: break;
          }
        }
        if (subSel == 4) vpidChange = (vpidChange + 1) % 4;
      }
      if (vpidChange != vpidSet) {
        int wopt = u8g2.userInterfaceMessage("Noicte", "VID PID apply", "after restart", " Cancel \n Ok \n Restart");
        switch (wopt) {
          case 1: break;
          case 2: vpidSet = vpidChange; sysSave(); break;
          case 3: vpidSet = vpidChange; sysSave(); forceReset();
          default: break;
        }
      }
    }
    if (sel == 2) {
      // bModeC = !bModeC;
      u8g2.userInterfaceMessage("Sorry", "Bluetooth is", "removed from code", " Ok ");
    }
    if (sel == 3) {
      wifiMenu();
    }
    else if (sel == 4) {
      int cMode = usbMode;
      const char mMenu[] = 
      "Keyboard\n"
      "Gamepad\n"
      "Mouse";
      cMode = noidMenu("USB Mode", cMode + 1, mMenu) - 1;
      if (cMode > -1 && cMode != usbMode) {
        int wopt = u8g2.userInterfaceMessage("Noicte", "USB Mode apply", "after restart", " Cancel \n Ok \n Restart");
        switch (wopt) {
          case 1: break;
          case 2: usbMode = cMode; sysSave(); break;
          case 3: usbMode = cMode; sysSave(); forceReset();
          default: break;
        }
      }
    }
  }
  if (bModeC != withBLE) {
  int wopt = u8g2.userInterfaceMessage("Noicte", "Bluetooth run", "after restart", " Cancel \n Ok \n Restart");
  switch (wopt) {
    case 1: break;
    case 2: withBLE = bModeC; sysSave(); break;
    case 3: withBLE = bModeC; sysSave(); forceReset();
    default: break;
  }
}
}

const String mList[8] = {
  "LCtrl",
  "LShift",
  "LAlt",
  "LGui",
  "RCtrl",
  "RShift",
  "RAlt",
  "RGui"
};

int modCount(uint8_t modi) {
  int count = 0;
  while (modi) {
    modi &= (modi - 1);
    count++;
  }
  return count;
}

void addMacroMenu(Macro &macro, int index) {
  int sel = 1;
  macType mType = MACRO_PRESS;
  uint8_t keycode = 0x04;
  uint8_t modifier = 0;
  String text = "hello!";
  unsigned long aDelay = 1000;
  while (sel > 0) {
    String tmp = "Type: ";
    switch (mType) {
      case MACRO_DELAY  : tmp += "Delay"; break;
      case MACRO_TEXT   : tmp += "Text"; break;
      case MACRO_PRESS  : tmp += "Press"; break;
      case MACRO_HOLD   : tmp += "Hold"; break;
      case MACRO_RELEASE: tmp += "Release"; break;
      default: tmp += "INVALID"; break;
    }
    tmp += "\nDelay: " + String(aDelay) + "ms";
    if (mType != MACRO_DELAY) tmp += "\n";
    switch (mType) {
      case MACRO_TEXT   : tmp += "Text: " + text; break;
      case MACRO_PRESS  : tmp += "Press: " + codeToName(keycode); break;
      case MACRO_HOLD   : tmp += "Hold: " + codeToName(keycode); break;
      case MACRO_RELEASE: tmp += "Release: " + codeToName(keycode); break;
      default: break;
    }
    if (mType >= MACRO_PRESS && mType <= MACRO_RELEASE) 
      tmp += "\nModifier: " + String(modCount(modifier));
    tmp += "\nConfirm";
    sel = noidMenu("Add Action", sel, tmp.c_str());
    switch (sel) {
    case 1: {
      const char ctype[] =
        "Press\n"
        "Hold\n"
        "Release\n"
        "Text\n"
        "Delay"
      ;
      int mt = noidMenu("Action Type", mType + 1, ctype);
      if (mt > 0) mType = (macType)(mt - 1);
      if (mType == MACRO_TEXT) aDelay = 20;
      break;
    }
    case 2:
      aDelay = valueSet("Delay ms", aDelay, true, (mType == MACRO_TEXT ? 5 : 100), 30000, true);
      break;
    case 3: {
      if (mType == MACRO_DELAY) {insertAct(macro, index, 0x00, MACRO_DELAY, aDelay); return;}
      if (mType >= MACRO_PRESS && mType <= MACRO_RELEASE) {
        int ckeycode = noidMenu("Change Key", codeToIndex(keycode), buttonName);
        if (ckeycode > 0) keycode = buttonCode[ckeycode - 1];
      } else {
        text = keyboard(text);
      }
      break;
    }
    case 4: {
      if (mType >= MACRO_PRESS && mType <= MACRO_RELEASE) {
        int subSel = 1;
        while (subSel > 0) {
          String tmp = "";
          for (int i = 0; i < 8; i++) {
            if (modifier & (1u << i)) tmp += "[*] ";
            else tmp += "[ ] ";
            tmp += mList[i];
            if (i < 7) tmp += "\n";
          }
          subSel = noidMenu("Modifier", subSel, tmp.c_str(), true);
          if (subSel > 0) modifier ^= (1 << subSel - 1);
        }
      } else {
        insertTextAct(macro, index, text.c_str(), aDelay); return;
      }
      break;
    }
    case 5: insertAct(macro, index, keycode, mType, aDelay, modifier); return;
    default: break;
    }
  }
}

void editMacroDelay(Macro &macro, int index) {
  int sel = 1;
  unsigned long aDelay = macro.actions[index].actDelay;
  while (sel > 0) {
    String tmp = "Delay: " + String(macro.actions[index].actDelay) + "ms"
    + "\nConfirm";
    sel = noidMenu("Edit Delay", 1, tmp.c_str());
    if (sel == 1) aDelay = valueSet("Delay (ms)", aDelay, true, 50, 30000, true);
    if (sel == 2) macro.actions[index].actDelay = aDelay; return;
  }
}

void editMacroPHR(Macro &macro, int index) {
  int sel = 1;
  uint8_t keycode = macro.actions[index].keycode;
  unsigned long aDelay = macro.actions[index].actDelay;
  uint8_t modifier = macro.actions[index].modifier;
  String typeShi = "";
  switch (macro.actions[index].mType) {
    case MACRO_RELEASE: typeShi = "Release"; break;
    case MACRO_PRESS:   typeShi = "Press";   break;
    case MACRO_HOLD:    typeShi = "Hold";    break;
  }
  while (sel > 0) {
    String tmp =
      "Keycode: " + codeToName(keycode)
    + "\nModifier: " + modCount(modifier)
    + "\nDelay: " + aDelay + "ms"
    + "\nConfirm";
    sel = noidMenu(("Edit " + typeShi).c_str(), sel, tmp.c_str());
    switch (sel) {
      case 1: {
        int ckeycode = noidMenu("Change Key", codeToIndex(keycode), buttonName);
        if (ckeycode > 0) keycode = buttonCode[ckeycode - 1];
        break;
      }
      case 2: {
        int subSel = 1;
        while (subSel > 0) {
          String tmp = "";
          for (int i = 0; i < 8; i++) {
            if (modifier & (1u << i)) tmp += "[*]";
            else tmp += "[ ]";
            tmp += mList[i];
            if (i < 7) tmp += "\n";
          }
          subSel = noidMenu("Modifier", subSel, tmp.c_str(), true);
          if (subSel > 0) modifier ^= (1 << subSel - 1);
        }
        break;
      }
      case 3: aDelay = valueSet("Delay (ms)", aDelay, true, 100, 30000, true); break;
      case 4: editAct(macro, index, keycode, macro.actions[index].mType, aDelay, modifier); return;
      default: break;
    }
  }
}

void editMacroText(Macro &macro, int index) {
  String text = String(macro.actions[index].text);
  unsigned long aDelay = macro.actions[index].actDelay;
  int sel = 1;
  while (sel > 0) {
    String tmp =
      "Text: " + text
    + "\nDelay: " + String(aDelay)
    + "\nConfirm";
    sel = noidMenu("Edit Text", sel, tmp.c_str());
    switch (sel) {
      case 1: text = keyboard(text); break;
      case 2: aDelay = valueSet("Delay (ms)", aDelay, true, 5, 30000, true);
      case 3: editTextAct(macro, index, text.c_str(), aDelay); return;
      default: break;
    }
  }
}

void macroMenu() {
  const char slots[] = 
    "Slot 1\n"
    "Slot 2\n"
    "Slot 3";
  int sel = 1;
  while (sel > 0) {
    sel = noidMenu("Macro", sel, slots);
    if (sel > 0) {
      Macro& mc = macQuick[sel - 1];
      const char menu[] = 
        "Edit Macro\n"
        "Run Macro\n"
        "Save Macro\n"
        "Load Macro\n"
        "Config";
      int subSel = 1;
      while (subSel > 0) {
        subSel = noidMenu(("Slot " + String(sel)).c_str(), subSel, menu);
        switch (subSel) {
        case 1: {
          int saw = 1;
          while (saw > 0) {
            String tmp = "";
            for (int i = 0; i < mc.macCount; i++) {
              uint8_t mt = mc.actions[i].mType;
              String mtn = "";
              switch (mt) {
                case MACRO_DELAY  : mtn = "Delay: " + String(mc.actions[i].actDelay) + "ms"; break;
                case MACRO_TEXT   : mtn = "Text: " + String(mc.actions[i].text); break;
                case MACRO_PRESS  : mtn = "Press: " + codeToName(mc.actions[i].keycode); break;
                case MACRO_HOLD   : mtn = "Hold: " + codeToName(mc.actions[i].keycode); break;
                case MACRO_RELEASE: mtn = "Release: " + codeToName(mc.actions[i].keycode); break;
                default: mtn = "INVALID"; break;
              }
              tmp += mtn + "\n";
            }
            tmp += "[Add]";
            saw = noidMenu("Edit Macro", saw, tmp.c_str());
            if (saw > 0 && saw - 1 < mc.macCount) {
              int subAct = noidMenu("Options", 1, "Edit\nInsert Here\nDelete");
              switch (subAct) {
                case 1: {
                  if (mc.actions[saw - 1].mType == MACRO_DELAY) editMacroDelay(mc, saw - 1);
                  else if (mc.actions[saw - 1].mType == MACRO_TEXT) editMacroText(mc, saw - 1);
                  else editMacroPHR(mc, saw - 1);
                  break;
                }
                case 2: addMacroMenu(mc, saw - 1); break;
                case 3: {
                  if (u8g2.userInterfaceMessage("Delete this", "action?", "", " Yes \n No ") == 1)
                    removeAct(mc, saw - 1);
                  break;
                }
                default: break;
              }
            }
            else if (saw > 0) {
              addMacroMenu(mc, mc.macCount);
            }
          }
          break;
        }
        case 2: {
          screenSaver(("Running Macro " + String(sel)).c_str());
          if (mc.rep == 0) executeMacro(mc, sel - 1);
          else if (mc.rep > 0) for (int i = 0; i < mc.rep; i++) executeMacro(mc, sel - 1);
          break;
        }
        case 3: {
          String mcrName = "New Macro";
          bool seling = true;
          while (seling) {
            const char actions[] =
              "New File\n"
              "Replace File"
            ;
            int act = noidMenu("Save Macro", 1, actions);
            if (act == 0) break;
            if (act == 1) {
              int opt = -1;
              while (opt != 0) {
                opt = u8g2.userInterfaceMessage(
                  "",
                  "Save Macro",
                  mcrName.c_str(),
                  " ch.Name \n Save "
                );
                if (opt == 1) mcrName = keyboard(mcrName);
                if (opt == 2) {
                  // remove characters not allowed in filenames
                  mcrName.replace("/", "-");
                  mcrName.replace("\\", "-");
                  mcrName.replace(":", "-");
                  mcrName.replace("*", "-");
                  mcrName.replace("?", "-");
                  mcrName.replace("\"", "-");
                  mcrName.replace("<", "-");
                  mcrName.replace(">", "-");
                  mcrName.replace("|", "-");
                  mcrName.trim();
                  if (mcrName.length() == 0) {
                    u8g2.clearBuffer();
                    u8g2.drawStr((128 - u8g2.getStrWidth("Invalid name!"))/2, 32, "Invalid name!");
                    u8g2.sendBuffer();
                    delay(1000);
                  }
                  else {
                    if (!mcrName.endsWith(".mcr")) mcrName += ".mcr";
                    mcrName = "/" + mcrName;
                    if (saveMacro(mcrName.c_str(), mc)) {
                      u8g2.userInterfaceMessage("Macro saved!", mcrName.c_str(), "", " Ok ");
                      seling = false;
                      opt = 0;
                    } else {
                      u8g2.userInterfaceMessage("Failed to save", mcrName.c_str(), "go back and try again", " Ok ");
                    }
                  }
                }
              }
            }
            if (act == 2) {
              std::vector<String> macros = listMacro();
              int macrosCount = macros.size();
              if (macrosCount) {
                String subSel = "";
                for (int i = 0; i < macrosCount; i++) {
                  subSel += macros[i];
                  if (i < macrosCount - 1) subSel += "\n";
                }
                int choose = noidMenu("Replace Macro", 1, subSel.c_str());
                if (choose != 0) {
                  mcrName = "/" + macros[choose - 1];
                  if (saveMacro(mcrName.c_str(), mc)) {
                    u8g2.userInterfaceMessage("Macro saved!", mcrName.c_str(), "", " Ok ");
                    seling = false;
                  } else {
                    u8g2.userInterfaceMessage("Failed to save", mcrName.c_str(), "", " Ok ");
                  }
                }
              } else {
                u8g2.userInterfaceMessage("Sorry but you haven't", "save any Macro yet", "", " Ok ");
              }
            }
          }         
          break;
        }
        case 4: {
          std::vector<String> macros = listMacro();
          int macrosCount = macros.size();
          if (macrosCount) {
            String subSel = "";
            for (int i = 0; i < macrosCount; i++) {
              subSel += macros[i];
              if (i < macrosCount - 1) subSel += "\n";
            }
            int choose = noidMenu("Load Macro", 0, subSel.c_str());
            if (choose != 0) {
              String mcrName = "/" + macros[choose - 1];
              if (loadMacro(mcrName.c_str(), mc)) {
                u8g2.userInterfaceMessage("Macro loaded!", mcrName.c_str(), "", " Ok ");
              } else {
                u8g2.userInterfaceMessage("Failed to load", mcrName.c_str(), "", " Ok ");
              }
            }
          } else {
            u8g2.userInterfaceMessage("Sorry but you haven't", "save any macro yet", "", " Ok ");
          }
          break;
        }
        case 5: {
          int sSel = 1;
          while (sSel > 0) {
            String tmp = 
              "Repeat: " + (mc.rep >= 0 ? String(mc.rep) : "until release")
            + "\nRelease to stop: " + (mc.iTr ? "Yes" : "No")
            + "\nQuick Path: " + String(prf.macPath[sel - 1])
            ;
            sSel = noidMenu("Macro Config", sSel, tmp.c_str());
            switch (sSel) {
            case 1: mc.rep = valueSet("Repeat", mc.rep, true, -1, 100, true); break;
            case 2: mc.iTr = !mc.iTr; break;
            case 3: {
              std::vector<String> macros = listMacro();
              int macrosCount = macros.size();
              if (macrosCount) {
                String subSel = "";
                for (int i = 0; i < macrosCount; i++) {
                  subSel += macros[i];
                  if (i < macrosCount - 1) subSel += "\n";
                }
                int choose = noidMenu("Load Macro", 0, subSel.c_str());
                if (choose != 0) {
                  String mcrName = "/" + macros[choose - 1];
                  strncpy(prf.macPath[sel - 1], mcrName.c_str(), sizeof(prf.macPath[sel - 1]) - 1);
                }
              }
              break;
            }
            default: break;
            }
          }
          break;
        }
        default:
          break;
        }
      }
    }
  }
}