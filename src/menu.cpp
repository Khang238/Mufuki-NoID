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

float valueSet(const char *title, float input, bool clamp, float clampMin, float clampMax) {
  #define SMTH_WAIT 150
  #define SMTH_FACC 1500
  #define SMTH_FAHH 5000
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
        input += (millis() - htA < SMTH_FAHH ? 0.01 : 0.81);
      }
    }
    else if (!digitalRead(btnPins[2])) {
      if (!hlB) {htB = millis(); hlB = true;}
      if (millis() - adw > SMTH_WAIT) {
        adw = millis() + ((millis() - htB < SMTH_FACC) ? 0 : SMTH_WAIT);
        input -= (millis() - htB < SMTH_FAHH ? 0.01 : 0.81);
      }
    }
    input += hallVal[2] - hallVal[0]; // fine-tune with hall input
    if (nowPress[1]) input = round(input); // snap to integer
    if (clamp) input = constrain(input, clampMin, clampMax);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    if (!digitalRead(btnPins[1])) {while (!digitalRead(btnPins[1])) delay(10); return input;}
    if (!digitalRead(btnPins[3])) {while (!digitalRead(btnPins[3])) delay(10); return sVal;}

    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 12, title);
    String tmp = String(input, 2);
    bool toNeg = hallVal[0] > hallVal[2];
    int smth = (int)((hallVal[2] - hallVal[0]) * 40);
    if (fabs(hallVal[2] - hallVal[0]) > 0.0) {
      u8g2.drawStr(64 + (int)((hallVal[2] - hallVal[0]) * 46) - 3, 52, toNeg ? "-" : "+");
      if (toNeg) u8g2.drawBox(64 + smth, 44, abs(smth), 8); 
      else       u8g2.drawBox(64, 44, abs(smth), 8);
    }
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 40, tmp.c_str());
    u8g2.setFont(u8g2_font_5x8_tr);
    if (clamp) {
      tmp = "Range: " + String(clampMin, 2) + " to " + String(clampMax);
    } else {
      tmp = "[no limit]";
    }
    u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 64, tmp.c_str());
    u8g2.sendBuffer();
  }
}

void calibMenu() {
  bool running = true;
  bool calib = false;
  int nowCal = 0;
  if (analogLed) ledcWrite(0, 2);
  else {b.setPixelColor(0, b.Color(255, 255, 255)); b.show();}
  for (int i = 0;  i < GRAPH_WIDTH; i++) graphData[i] = {0};
  while (running) {
    // updateInput();
    if (calib) {
      calMin[nowCal] = (calMin[nowCal] < rawVal[nowCal]) ? calMin[nowCal] : rawVal[nowCal];
      calMax[nowCal] = (calMax[nowCal] > rawVal[nowCal]) ? calMax[nowCal] : rawVal[nowCal];
    }
    switch (getButton())
    {
    case 0:
      nowCal = (nowCal + 2) % 3;
      for (int i = 0;  i < GRAPH_WIDTH; i++)
        graphData[i] = {0};
      for (int i = 0; i < 3; i++) {
        if (i == nowCal)
          if (analogLed) ledcWrite(i, 2);
          else b.setPixelColor(i, b.Color(255, 255, 255));
        else
          if (analogLed) ledcWrite(i, 0);
          else b.setPixelColor(i, b.Color(0, 0, 0));
      }
      if (calib) {
        calMin[nowCal] = 4095;
        calMax[nowCal] = 0;
      }
      break;
    case 1:
      calib = !calib;
      if (calib) {
        calMin[nowCal] = 4095;
        calMax[nowCal] = 0;
      }
      break;
    case 2:
      nowCal = (nowCal + 1) % 3;
      for (int i = 0;  i < GRAPH_WIDTH; i++)
        graphData[i] = {0};
      for (int i = 0; i < 3; i++) {
        if (i == nowCal)
          if (analogLed) ledcWrite(i, 2);
          else b.setPixelColor(i, b.Color(255, 255, 255));
        else
          if (analogLed) ledcWrite(i, 0);
          else b.setPixelColor(i, b.Color(0, 0, 0));
      }
      if (calib) {
        calMin[nowCal] = 4095;
        calMax[nowCal] = 0;
      }
      break;
    case 3:
      running = false;
    default:
      break;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr(0, 12, ((calib ? "Calibrating S" : "Viewing S") + String(nowCal + 1)).c_str());
    u8g2.drawStr(0, 24, ("+ " + String((int)calMax[nowCal])).c_str());
    u8g2.drawStr(0, 35, ("p " + String((int)rawVal[nowCal])).c_str());
    u8g2.drawStr(0, 46, ("- " + String((int)calMin[nowCal])).c_str());
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(108, 12, String(hallVal[nowCal]).c_str());
    u8g2.drawStr(0, 56, "F1: Last"); u8g2.drawStr(50, 56, calib ? "F2: Stop Calib" : "F2: Calibrate");
    u8g2.drawStr(0, 64, "F3: Next"); u8g2.drawStr(50, 64, "F4: Exit");
    pushGraphValue(hallVal[nowCal]);
    drawGraph(40, 14);
    if (calib)
      u8g2.drawLine(40, (int)((1.0 - hallVal[nowCal]) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - hallVal[nowCal]) * (GRAPH_HEIGHT - 1)) + 14);
    else
      switch (inputHandler) {
        case 0: u8g2.drawLine(40, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14); break;
        case 1:
          u8g2.drawLine(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        case 2:
          u8g2.drawLine(40, (int)((1.0 - windowFoot[nowCal]) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - windowFoot[nowCal]) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - (windowFoot[nowCal] + windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (windowFoot[nowCal] + windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
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
  //for (int i = 0; i < 3; i++) {
  //  calMin[i] = calMin[i] + deadZone;
  //  calMax[i] = calMax[i] - deadZone;
  //}
}

void inputMenu() {
  bool running = true;
  int hysteresisChange = 0;
  bool hysteresisChanging = false;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    graphData[i] = 0;
  }
  while (running) {
    u8g2.clearBuffer();
    float maxPress = max(max(hallVal[0], hallVal[1]), hallVal[2]);
    float maxFoot = max(max(windowFoot[0], windowFoot[1]), windowFoot[2]);
    pushGraphValue(maxPress);
    drawGraph(40, 14);
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    switch (inputHandler) {
      case 0:
      inputTypeDigitalEmulation();
        u8g2.drawStr(0, 12, "DigitalEmulation");
        u8g2.drawStr(0, 24, "Acc:");
        u8g2.drawStr(0, 36, String(actuation).c_str());
        u8g2.drawLine(40, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14);
        break;
      case 1:
        inputTypeHysteresisHandling();
        u8g2.drawStr(0, 12, "Hysteresis");
        u8g2.drawStr(0, 24, ("U: " + String(upperThreshold)).c_str());
        u8g2.drawStr(0, 36, ("L: " + String(lowerThreshold)).c_str());
        u8g2.drawLine(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        u8g2.drawLine(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        if (hysteresisChange == 0) {
          u8g2.drawTriangle(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 10, 40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 18, 46, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        } else {
          u8g2.drawTriangle(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 10, 40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 18, 46, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
        }
        break;
      case 2:
        inputTypeDynamicActuation();
        u8g2.drawStr(0, 12, "DynamicAct");
        u8g2.drawStr(0, 24, ("W: " + String(windowSize)).c_str());
        u8g2.drawStr(0, 36, ("F: " + String(maxFoot)).c_str());
        u8g2.drawLine(40, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14, 128  , (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14);
        u8g2.drawLine(40, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
        break;
      default:
        inputTypeDigitalEmulation();
        break;
    }
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(108, 12, String(maxPress).c_str());
    switch (inputHandler) {
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
        if (inputHandler == 0) {
          actuation += 0.05;
          actuation = constrain(actuation, 0.05, 0.95);
        } else if (inputHandler == 1) {
          if (!hysteresisChanging) {
            hysteresisChange = (hysteresisChange + 1) % 2;
          }
          else if (hysteresisChange == 0) {
            upperThreshold += 0.05;
            upperThreshold = constrain(upperThreshold, lowerThreshold + 0.05, 0.95);
          }
          else if (hysteresisChange == 1) {
            lowerThreshold += 0.05;
            lowerThreshold = constrain(lowerThreshold, 0.05, upperThreshold - 0.05);
          }
        } else if (inputHandler == 2) {
          windowSize += 0.05;
          windowSize = constrain(windowSize, 0.05, 1.00);
        }
        break;
      case 1:
        if (!(hysteresisChanging && inputHandler == 1))
          inputHandler = (inputHandler + 1) % 3;
        else
          hysteresisChanging = false;
        break;
      case 2:
        if (inputHandler == 0) {
          actuation -= 0.05;
          actuation = constrain(actuation, 0.05, 0.95);
        } else if (inputHandler == 1) {
          if (!hysteresisChanging) {
            hysteresisChanging = true;
          }
          else if (hysteresisChange == 0) {
            upperThreshold -= 0.05;
            upperThreshold = constrain(upperThreshold, lowerThreshold + 0.05, 0.95);
          }
          else if (hysteresisChange == 1) {
            lowerThreshold -= 0.05;
            lowerThreshold = constrain(lowerThreshold, 0.05, upperThreshold - 0.05);
          }
        } else if (inputHandler == 2) {
          windowSize -= 0.05;
          windowSize = constrain(windowSize, 0.05, 1.00);
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

void filtMenu() {
  bool running = true;
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    graphData[i] = 0;
  }
  while (running) {
    // updateInput();
    float maxHall = max(max(hallVal[0], hallVal[1]), hallVal[2]);
    float maxFoot = max(max(windowFoot[0], windowFoot[1]), windowFoot[2]);
    int btn = getButton();
    switch (btn) {
      case 0: filterType = 0; break;
      case 1: doFilter = !doFilter; break;
      case 2: filterType = 1; break;
      case 3: running = false; break;
      default: break;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr(0, 12, doFilter ? "Filter" : "Filter [Off]");
    u8g2.drawStr(0, 24, ("1 " + String((int)rawVal[0])).c_str());
    u8g2.drawStr(0, 35, ("2 " + String((int)rawVal[1])).c_str());
    u8g2.drawStr(0, 46, ("3 " + String((int)rawVal[2])).c_str());
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(108, 12, String(maxHall).c_str());
    u8g2.drawStr(0, 56, filterType == 0 ? "> F1: OVS" : "F1: OV"); u8g2.drawStr(50, 56, doFilter ? "F2: Disable" : "F2: Enable");
    u8g2.drawStr(0, 64, filterType == 1 ? "> F3: EMA" : "F3: AV"); u8g2.drawStr(50, 64, "F4: Exit");
    pushGraphValue(maxHall);
    drawGraph(40, 14);
      switch (inputHandler) {
        case 0: u8g2.drawLine(40, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - actuation) * (GRAPH_HEIGHT - 1)) + 14); break;
        case 1:
          u8g2.drawLine(40, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - upperThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - lowerThreshold) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        case 2:
          u8g2.drawLine(40, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - maxFoot) * (GRAPH_HEIGHT - 1)) + 14);
          u8g2.drawLine(40, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14, 128, (int)((1.0 - (maxFoot + windowSize)) * (GRAPH_HEIGHT - 1)) + 14);
          break;
        default: break;
      }
    u8g2.sendBuffer();
  }
  if (doFilter) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    int curious = u8g2.userInterfaceMessage(
      "ADC may change",
      "slower while display",
      "is working",
      " Ok \n More "
    ); // for later info
  }
}

void effectMenu() {
  int sel = 1;
  while (sel != 0) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    String menu_items =
      "Under Glow " + String(underGlow ? "[On]" : "[Off]") + "\n"
      + "RGB Led " + String(rgb ? "[On]" : "[Off]");
    sel = u8g2.userInterfaceSelectionList("Effects", sel, menu_items.c_str());
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
        "Under Glow: " + String(underGlow ? "[On]" : "[Off]") + "\n"
        + "Type: " + effectsList[glowType] + "\n";
        for (int i = 0; i < 6; i++) {
          tmp += (glowType == i ? "> " : "") + effectsList[i] + (glowType == i ? " <" : "");
          if (i < 5) tmp += "\n";
        }
        subSel = u8g2.userInterfaceSelectionList("Under Glow", subSel, tmp.c_str());
        if (subSel == 1) underGlow = !underGlow;
        else if (subSel >= 3 && subSel <= 8) glowType = subSel - 3;
      }
    } else if (sel == 2) {
      int subSel = 1;
      while (subSel != 0){
        String menuElements =
          "RGB Led: " + String(rgb ? "[On]" : "[Off]") + "\n"
          + "Brightness: " + String(rgbBri) + "\n"
          + "Mode: " + String(doRainbow ? "Rainbow" : "Static") + "\n";
        if (doRainbow) {
          menuElements += "Speed: " + String(rainbowStep) + "\n";
          menuElements += "Interval: " + String(rgbInterval * 10) + " ms";
        } else {
          menuElements += "Color (" + String(color[0]) + ", " + String(color[1]) + ", " + String(color[2]) + ")";
        }
        subSel = u8g2.userInterfaceSelectionList("RGB Led", subSel, menuElements.c_str());
        if (subSel == 1) rgb = !rgb;
        if (subSel == 2) rgbBri = (uint8_t)valueSet("Brightness", rgbBri, true, 0, 255);
        // u8g2.user.InterfaceInputValue("Brightness\n", "(0 - 255): ", &rgbBri, 0, 255, 3, " ");
        if (subSel == 3) doRainbow = !doRainbow;
        if (subSel == 4) {
          if (doRainbow) {
            //u8g2.user.InterfaceInputValue("Speed\n", "(1 - 255): ", &rainbowStep, 1, 255, 3, " ");
            rainbowStep = (uint8_t)valueSet("Speed", rainbowStep, true, 1, 255);
          } else {
            int sub2Sel = 1;
            while (sub2Sel != 0) {
              String colorElements =
                + "R: " + String(color[0]) + "\n"
                + "G: " + String(color[1]) + "\n"
                + "B: " + String(color[2]);
              sub2Sel = u8g2.userInterfaceSelectionList("Color\n", sub2Sel, colorElements.c_str());
              if (sub2Sel == 1) color[0] = (uint8_t)valueSet("Red", color[0], true, 0, 255);
              else if (sub2Sel == 2) color[1] = (uint8_t)valueSet("Blue", color[1], true, 0, 255);
              else if (sub2Sel == 3) color[2] = (uint8_t)valueSet("Green", color[2], true, 0, 255);
            }
          }
        }
        if (subSel == 5) rgbInterval = (uint8_t)valueSet("Interval", rgbInterval, true, 1, 255);
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
    "Brightness: " + String(screenBri) + "\n"
    + "Screen on: " + String(screenSaveDuration / 1000) + "s\n"
    + "Screen save: " + String((screenOffDuration) / 1000) + "s\n";
    if (logoType == 0 || logoType == 12)
      menuItem += "Icon: \"" + screenLogo + "\"";
    else
      menuItem += "Icon: \"" + String(kaoOrSomethingIdk[logoType - 1]) + "\"";
    subSel = u8g2.userInterfaceSelectionList("Display", subSel, menuItem.c_str());
    if (subSel == 1) {screenBri = (uint8_t)valueSet("Brightness", screenBri, true, 0, 255); u8g2.setContrast(screenBri);}
    if (subSel == 2) {
      switch (u8g2.userInterfaceSelectionList("Screen on", 1, lazyAss)) {
        case 1: screenSaveDuration = 1000; screenOffDuration += 1000; break;
        case 2: screenSaveDuration = 5000; break;
        case 3: screenSaveDuration = 10000; break;
        case 4: screenSaveDuration = 20000; break;
        case 5: screenSaveDuration = 30000; break;
        case 6: screenSaveDuration = 60000; break;
        case 7: screenSaveDuration = 300000; break;
        case 8: screenSaveDuration = 600000; break;
        default: break;
      }
    }
    if (subSel == 3) {
      switch (u8g2.userInterfaceSelectionList("Screen on", 1, lazyAss)) {
        case 1: screenOffDuration = 1000; break;
        case 2: screenOffDuration = 5000; break;
        case 3: screenOffDuration = 10000; break;
        case 4: screenOffDuration = 20000; break;
        case 5: screenOffDuration = 30000; break;
        case 6: screenOffDuration = 60000; break;
        case 7: screenOffDuration = 300000; break;
        case 8: screenOffDuration = 600000; break;
        default: break;
      }
    }
    if (subSel == 4) {
      String tmp = "";
      for (int i = 0; i < 12; i++) {
        tmp += kaoOrSomethingIdk[i];
        if (i < 11) tmp += "\n";
      }
      int logSel = u8g2.userInterfaceSelectionList("Display Icon", 1, tmp.c_str());
      if (logSel == 1) {
        screenLogo = "Mufuki";
        logoType = 0;
      } else if (logSel > 1 && logSel < 12) logoType = logSel;
      else if (logSel != 0) {
        screenLogo = keyboard(screenLogo);
        logoType = 12;
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
    sel = u8g2.userInterfaceSelectionList("MPU", sel, useless);
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
          u8g2.setFont(u8g2_font_fub20_tf);
          u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 40, tmp.c_str());
          u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
          tmp = "Min" + String(i + 1) + ":" + String(int(calMin[i]));
          u8g2.drawStr(0, 40 + i * 10, tmp.c_str());
          tmp = "Max" + String(i + 1) + ":" + String(int(calMax[i]));
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
        tmp = "adcDeadZone:" + String(deadZone);
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
    option = u8g2.userInterfaceSelectionList("DeadZone Calibrate", option, menuItems);
    if (option == 1) {
      for (int i = 0; i < 5; i++){
        u8g2.clearBuffer();
        u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
        u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
        String tmp = "<<<" + String(5 - i) + "s>>>";
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
        u8g2.clearBuffer();
        u8g2.drawStr((128 - u8g2.getStrWidth("Calibrating..."))/2, 28, "Calibrating...");
        u8g2.drawStr((128 - u8g2.getStrWidth("Please dont touch!"))/2, 40, "Please dont touch!");
        String tmp = "[" + String(sw + 1) + "/3]";
        u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 60, tmp.c_str());
        u8g2.sendBuffer();
        for (int i = 0; i < 2000; i++) {
          int adcVal = 0;
          if (doFilter) {
            if (filterType == 0) {
              adcVal = overSample(sw);
            } else {
              adcVal = expoMovAvr(sw);
            }
          } else adcVal = analogRead(adcPins[sw]);
          if (adcVal > aMax) aMax = adcVal;
          if (adcVal < aMin) aMin = adcVal;
          delay(1);
        }
      }
      deadZone = aMax - aMin;
      int avrgMin = (calMin[0] + calMin[1] + calMin[2]) / 3;
      int avrgMax = (calMax[0] + calMax[1] + calMax[2]) / 3;
      if (avrgMax - avrgMin - deadZone * 2 < 100) {
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
            ("DeadZone: " + String(deadZone)).c_str(),
            ("Dynamic Range: " + String(avrgMax - avrgMin - deadZone * 2)).c_str(),
            " Next>> "
          );
        }
      }
      u8g2.userInterfaceMessage(
        "Calibration done!",
        ("Dead Zone: " + String(deadZone)).c_str(),
        "",
        " Ok "
      );
    }
    else if (option == 2) {
      deadZone = (int)valueSet("Dead Zone", deadZone, true, 0, 4095);
    }
  }
  u8g2.userInterfaceMessage(
    "Remember to",
    "go to calibration",
    "menu to recalibrate",
    " Ok "
  );
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
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(0, 25, t1);
  u8g2.drawStr(0, 37, t2);
  if (dobtn)
    u8g2.drawButtonUTF8(120 - u8g2.getStrWidth(btn), 64 - u8g2.getMaxCharHeight(), U8G2_BTN_INV, 0,  2,  2, btn);
  u8g2.sendBuffer();
  waitAction(waitKey);
}

void firstTimeSetup() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub20_tf);
  for (int i = 0; i < 41; i++) {
    u8g2.clearBuffer();
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, i, "Mufuki");
    u8g2.sendBuffer();
    //delay(8);
  }
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
  splScreen("Buttons", "Side buttons", "F1 to F4 ---->", " Next ");
  splScreen("Buttons", "F1: Up     F2: OK", "F3: Down  F4:Back", " Next ");
  splScreen("Switches", "SW1 to SW3", "with hall sensors", " Next ");
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
      doFilter = true;
      filterType = 1;
      inputHandler = 2;
      windowSize = 0.25;
      rgb = true;
      color[0] = 255; 
      color[1] = 1;
      color[2] = 224;
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
  splScreen("Setup Complete!", "Web app also", "available in menu", " Next ");
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
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr((128 - u8g2.getStrWidth("Clearing Disk..."))/2, 28, "Clearing Disk...");
  u8g2.sendBuffer();
  LittleFS.end();

  if (LittleFS.format()) {
    u8g2.clearBuffer();
    u8g2.drawStr((128 - u8g2.getStrWidth("Clear"))/2, 28, "Clear");
    delay(1000);
  }
  forceReset();
}

void systemMenu() {
  int sel = 1;
  const char menuItems[] =
    "Display\n"
    "Deadzone Calibrate\n"
    "Profiles\n"
    "MPU\n"
    "Reset\n"
    "Debug"
  ;
  while (sel != 0) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("System", sel, menuItems);
    switch (sel) {
      case 1: displaySetting(); break;
      case 2: deadCalib(); break;
      case 3: profileMenu(); break;
      case 4: mpuMenu(); break;
      case 5: fomartFS(); break;
      case 6: showDebug(); break;
      default: break;
    }
  }
}

void about() {
  l.setBrightness(255);
  l.fill(l.Color(255, 0, 255));
  l.show();
  bool running = true;
  for (int i = 0; i < 10; i++) {
    u8g2.clearBuffer();
    u8g2.drawXBMP(20, 20, 88, 232, logoKao[i]);
    u8g2.setDrawColor(0);
    u8g2.drawBox(20, 0, 88, 20);
    u8g2.drawBox(20, 40, 88, 24);
    u8g2.setDrawColor(1);
    u8g2.sendBuffer();
    delay(500);
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub20_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
  u8g2.sendBuffer();
  delay(1000);
  for (int i = 0; i < 23; i++) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth(ver.c_str()))/2, 76 - i, ver.c_str());
    u8g2.sendBuffer();
    delay(20);
  }
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 52, 128, 12);
  u8g2.setDrawColor(1);
  u8g2.drawStr((128 - u8g2.getStrWidth((ver + " - by NoID").c_str()))/2, 54, (ver + " - by NoID").c_str());
  u8g2.sendBuffer();
  uint16_t h = 0;
  while (running) {
    // updateInput();
    for (int i = 0; i < 3; i++) {
      ledOutput[i] = 0;
      if (applyEffect[i]) {
        singleFade[i].active = true;
        singleFade[i].startTime = millis();
        applyEffect[i] = false;
      }
    }
    updateSingleFade();
    underGlowUpdate();
    h += 256;
    l.rainbow(h, 1, 255, rgbBri, true);
    l.show();
    if (getButton() == 3) running = false;
    delay(60);
  }
  l.setBrightness(0);
  l.show();
}

void wifiConnectScreen() { // separate function to save flash
  unsigned long beginTime = millis();
  while (millis() - beginTime < 60000 && WiFi.status() != WL_CONNECTED) {
    int btnPress = getButton();
    if (btnPress == 3) beginTime = millis() - 62000;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 34, "Mufuki");
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
  u8g2.setFont(u8g2_font_fub20_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 34, "Mufuki");
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
    subSel = u8g2.userInterfaceSelectionList("WiFi Settings", subSel, wItems.c_str());
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
        u8g2.setFont(u8g2_font_fub20_tf);
        u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
        u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
            wsSel = u8g2.userInterfaceSelectionList("Select Network", wsSel, Wlist.c_str());
            if (wsSel == 1) {
              u8g2.clearBuffer();
              u8g2.setFont(u8g2_font_fub20_tf);
              u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
              u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
          wsSel = u8g2.userInterfaceSelectionList("Manual connect", wsSel, wConItems.c_str());
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

void connectMenu() {
  const char items[] =
    "USB HID\n"
    "Bluetooth\n"
    "WiFi Settings\n"
    "Keyboard Layout";
  int sel = 1;
  int vpidChange = vpidSet;
  while (sel != 0) {
    sel = u8g2.userInterfaceSelectionList("Connection", sel, items);
    if (sel == 1) {
      int subSel = 1;
      while (subSel != 0) {
        String usbItems =
          "Status: " + String(tud_ready() ? "[OK]" : "[Offline]") + "\n"
          "Always Report: " + String(alwaysReport ? "[On]" : "[Off]") + "\n"
          "Polling Rate: " + String(1000000 / LOOP_INTERVAL_US) + " Hz\n"
          "Mode: ";
        switch (usbMode) {
        case 0: usbItems += "Keyboard"; break;
        case 1: usbItems += "Gamepad"; break;
        case 2: usbItems += "Mouse"; break;
        default: break;
        }
        if (usbMode == 1) {
          usbItems += "\nVID PID Set: ";
          switch (vpidChange) {
            case 0: usbItems += "PS4"; break;
            case 1: usbItems += "Xbox"; break;
            case 2: usbItems += "Switch"; break;
            case 3: usbItems += "Logitech"; break;
            default: break;
          }
        }
        subSel = u8g2.userInterfaceSelectionList("USB HID", subSel, usbItems.c_str());
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
            "8000 Hz [!]";
          int pollSel = u8g2.userInterfaceSelectionList("Polling Rate", 1, pollingList);
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
                "8000Hz is unstable",
                "Continue?",
                " Yes \n No "
              );
              if (confirm == 1) LOOP_INTERVAL_US = 125;
              break;
            }
            default: break;
          }
        }
        if (subSel == 4) {
          int cMode = usbMode;
          const char mMenu[] = 
          "Keyboard\n"
          "Gamepad\n"
          "Mouse";
          cMode = u8g2.userInterfaceSelectionList("USB Mode", cMode + 1, mMenu) - 1;
          if (cMode != usbMode) {
            int wopt = u8g2.userInterfaceMessage("Noicte", "USB Mode apply", "after restart", " Cancel \n Ok \n Restart");
            switch (wopt) {
              case 1: break;
              case 2: usbMode = cMode; sysSave(); break;
              case 3: usbMode = cMode; sysSave(); forceReset();
              default: break;
            }
          }
        }
        if (subSel == 5) vpidChange = (vpidChange + 1) % 4;
      }
    }
    if (sel == 2) {
      int subSel = 1;
      subSel = u8g2.userInterfaceMessage(
        "Attention [1 / 2]",
        "BLE function is not",
        "stable",
        " Next>> "
      );
      if (subSel == 0) continue;
      subSel = u8g2.userInterfaceMessage(
        "Attention [2 / 2]",
        "Make sure to calli-",
        "brate before use",
        " Ok "
      );
      if (subSel == 0) continue;
      int BLEfunction = -1;
      while (subSel != 0) {
        String bleItems =
          "Device Name: " + btName + "\n"
          + "--------- Mode ---------\n"
          + "Keyboard\n"
          + "Air Mouse\n"
          + "Gamepad";
        subSel = u8g2.userInterfaceSelectionList("Bluetooth", subSel, bleItems.c_str());
        if (subSel == 1) {
          btName = keyboard(btName);
        } else {
          BLEfunction = subSel - 3;
          subSel = 0;
        }
      }
      if (BLEfunction == 0) mwk();
      else if (BLEfunction == 1) marm();
      else if (BLEfunction == 2) mgp();
      u8g2.setPowerSave(0);
    }
    if (sel == 3) {
      wifiMenu();
    }
    else if (sel == 4) {
      int layChange = 1;
      while (layChange != 0) {
        String layoutName[] = {
          "Standart",
          "WASD",
          "Arrows",
          "Shortcuts",
          "For BLE Key",
          "Custom"
        };
        String tmpName = "";
        for (int i = 0; i < 6; i++) {
          tmpName += (i == layoutType ? "> " : "") + layoutName[i] + (i == layoutType ? " <" : "");
          if (i < 5) tmpName += "\n";
        }
        layChange = u8g2.userInterfaceSelectionList("Layout", layoutType + 1, tmpName.c_str());
        if (layChange == 0) continue;
        layoutType = layChange != 0 ? layChange - 1 : layoutType;
        if (layoutType == 5) {
          int layoutSel = 1;
          while (layoutSel != 0) {
            String nowLayout = "";
            for (int i = 0; i < 6; i++) {
              if (i < 3) nowLayout += "Sw" + String(i + 1) + ": ";
              else nowLayout += "F" + String(i - 2) + ": ";
              nowLayout += codeToName(layout[i]);
              if (i < 5) nowLayout += "\n";
            }
            layoutSel = u8g2.userInterfaceSelectionList("Custom Layout", layoutSel, nowLayout.c_str());
            if (layoutSel > 0) {
              int ckeycode = u8g2.userInterfaceSelectionList("Change Key", 1, buttonName);
              layout[layoutSel - 1] = buttonCode[ckeycode - 1];
            }
          }
        } else {
          layChange = 0;
          for (int i = 0; i < 6; i++) {
            layout[i] = preLayout[layoutType][i];
          }
        }
      }
    }
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