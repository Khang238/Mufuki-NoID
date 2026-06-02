#include "sandbox.h"

Profile testProfile;

OutputState mOutputs;

float valueSet(const char *title, float input, bool clamp, float clampMin, float clampMax) {
  #define SMTH_WAIT 150
  #define SMTH_FACC 1500
  #define SMTH_FAHH 5000
  unsigned long htA = 0, htB = 0, adw = 0;
  bool hlA = false, hlB = false;
  float sVal = input;
  while (!digitalRead(btnPins[2])) delay(10);
  while (true) {
    updateInput();
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
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 40, tmp.c_str());
    u8g2.setFont(u8g2_font_5x8_mf);
    if (clamp) {
      tmp = "Range: " + String(clampMin, 2) + " to " + String(clampMax);
    } else {
      tmp = "[no limit]";
    }
    u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 64, tmp.c_str());
    u8g2.sendBuffer();
  }
}

// Axis: chỉ cho axis targets
static const char* axisOnlyDstLabels[] = {
  "LX","LY","RX","RY","LT","RT",
  "MsX","MsY","MsW"
};
static const OutputTarget axisOnlyDstCodes[] = {
  OUT_AXIS_LX, OUT_AXIS_LY, OUT_AXIS_RX, OUT_AXIS_RY, OUT_AXIS_LT, OUT_AXIS_RT,
  OUT_MOUSE_X, OUT_MOUSE_Y, OUT_MOUSE_WHEEL
};
static const int axisOnlyDstCount = 9;

// Threshold: chỉ cho button/key targets
static const char* threshOnlyDstLabels[] = {
  "Key","GPBtn","MsBtn"
};
static const OutputTarget threshOnlyDstCodes[] = {
  OUT_KEY, OUT_BTN_GP, OUT_MOUSE_BTN
};
static const int threshOnlyDstCount = 3;

void axsAdd(Profile& p) {
  uint8_t srce = 0, destIdx = 0;  // destIdx → index trong axisOnlyDst
  float im = 0.0f, ix = 1.0f, om = -127.0f, ox = 127.0f;
  bool cmp = true;
  uint8_t cmb = COMBINE_NONE;
  int sel = 1;

  while (sel != 0) {
    bool imuSrc = (srce >= SRC_GYRO_X);

    String opts =
      "Input:  " + String(srcLabels[srce]) + " [" + String(im,0) + "~" + String(ix,0) + "]\n"
      "Output: " + String(axisOnlyDstLabels[destIdx]) + " [" + String(om,0) + "~" + String(ox,0) + "]\n"
      "Clamp: "  + String(cmp ? "Yes" : "No") + "\n"
      "Combine: ";
    switch (cmb) {
      case COMBINE_NONE: opts += "None"; break;
      case COMBINE_POS:  opts += "+";    break;
      case COMBINE_NEG:  opts += "-";    break;
      default: break;
    }
    opts += "\nConfirm";

    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("Add Axis Map", sel, opts.c_str());

    switch (sel) {
      case 1: { // Input — không đổi
        int ssel = 1;
        while (ssel != 0) {
          String sopt =
            "Source: " + String(srcLabels[srce]) + "\n"
            "Min: "    + String(im, 2) + "\n"
            "Max: "    + String(ix, 2);
          ssel = u8g2.userInterfaceSelectionList("Input", ssel, sopt.c_str());
          switch (ssel) {
            case 1: {
              String sList = "";
              for (int i = 0; i < SRC_COUNT; i++)
                sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
              int pick = u8g2.userInterfaceSelectionList("Input Source", srce + 1, sList.c_str());
              if (pick > 0) {
                srce = pick - 1;
                if (srce < 6)       { im = 0.0f;    ix = 1.0f;   }
                else if (srce < 9)  { im = -500.0f; ix = 500.0f; }
                else if (srce < 12) { im = -2.0f;   ix = 2.0f;   }
                else                { im = -180.0f; ix = 180.0f; }
              }
              break;
            }
            case 2: im = valueSet("Min Input", im, false/*!imuSrc*/, imuSrc ? -500.0f : 0.0f, ix);  break;
            case 3: ix = valueSet("Max Input", ix, false/*!imuSrc*/, im, imuSrc ? 500.0f : 1.0f);   break;
            default: break;
          }
        }
        break;
      }

      case 2: { // Output — chỉ dùng axisOnlyDst
        int ssel = 1;
        while (ssel != 0) {
          String sopt =
            "Target: " + String(axisOnlyDstLabels[destIdx]) + "\n"
            "Min: "    + String(om, 2) + "\n"
            "Max: "    + String(ox, 2);
          ssel = u8g2.userInterfaceSelectionList("Output", ssel, sopt.c_str());
          switch (ssel) {
            case 1: {
              String dList = "";
              for (int i = 0; i < axisOnlyDstCount; i++)
                dList += String(axisOnlyDstLabels[i]) + (i < axisOnlyDstCount - 1 ? "\n" : "");
              int pick = u8g2.userInterfaceSelectionList("Output Target", destIdx + 1, dList.c_str());
              if (pick > 0) {
                destIdx = pick - 1;
                // mouse axes nhỏ hơn gamepad axes
                OutputTarget dst = axisOnlyDstCodes[destIdx];
                if (dst == OUT_MOUSE_X || dst == OUT_MOUSE_Y || dst == OUT_MOUSE_WHEEL)
                  { om = -127.0f; ox = 127.0f; }
                else
                  { om = -127.0f; ox = 127.0f; } // gamepad cũng -127~127
              }
              break;
            }
            case 2: om = valueSet("Min Output", om, false, -127.0f, ox);  break;
            case 3: ox = valueSet("Max Output", ox, false, om,  127.0f);  break;
            default: break;
          }
        }
        break;
      }

      case 3: cmp = !cmp; break;

      case 4: {
        const char* cmbOpts = "None\nAdd (+)\nSubtract (-)";
        int pick = u8g2.userInterfaceSelectionList("Combine Mode", cmb + 1, cmbOpts);
        if (pick > 0) cmb = pick - 1;
        break;
      }

      case 5:
        addAxisMapping(p, (InputSource)srce, axisOnlyDstCodes[destIdx],
                       im, ix, om, ox, cmp, (CombineMode)cmb);
        return;

      default: break;
    }
  }
}

// ---- thrAdd ----
void thrAdd(Profile& p) {
  uint8_t srce = 0, destIdx = 0;
  float pos = 0.5f, neg = 0.0f, abs_ = 0.0f;
  uint8_t kc = 0;
  uint8_t cmb = COMBINE_NONE;
  int sel = 1;

  while (sel != 0) {
    String opts =
      "Input:   " + String(srcLabels[srce]) + "\n"
      "Target:  " + String(threshOnlyDstLabels[destIdx]) + "\n"
      "Pos thr: " + String(pos, 2) + "\n"
      "Neg thr: " + String(neg, 2) + "\n"
      "Abs thr: " + String(abs_, 2) + "\n"
      "Keycode: " + (destIdx == 0 ? codeToName(kc) : String(kc)) + "\n"
      "Confirm";

    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("Add Threshold", sel, opts.c_str());

    switch (sel) {
      case 1: { // Input source
        String sList = "";
        for (int i = 0; i < SRC_COUNT; i++)
          sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
        int pick = u8g2.userInterfaceSelectionList("Input Source", srce + 1, sList.c_str());
        if (pick > 0) srce = pick - 1;
        break;
      }
      case 2: { // Target
        String dList = "";
        for (int i = 0; i < threshOnlyDstCount; i++)
          dList += String(threshOnlyDstLabels[i]) + (i < threshOnlyDstCount - 1 ? "\n" : "");
        int pick = u8g2.userInterfaceSelectionList("Output Target", destIdx + 1, dList.c_str());
        if (pick > 0) destIdx = pick - 1;
        break;
      }
      case 3: pos  = valueSet("Pos Threshold", pos,  false, 0.0f, 500.0f); break;
      case 4: neg  = valueSet("Neg Threshold", neg,  false, 0.0f, 500.0f); break;
      case 5: abs_ = valueSet("Abs Threshold", abs_, false, 0.0f, 500.0f); break;
      case 6: {
        if (destIdx == 0) { // OUT_KEY thì chọn keycode, còn OUT_BTN_GP / OUT_MOUSE_BTN thì chọn button index (cũng lưu trong keycode)
          int pick = u8g2.userInterfaceSelectionList("Keycode", codeToIndex(kc), buttonName);
          if (pick > 0) kc = buttonCode[pick - 1];
        } else {
          String bList = "";
          uint8_t maxBtn = (threshOnlyDstCodes[destIdx] == OUT_BTN_GP) ? 16 : 8; // tối đa 16 nút cho gamepad, 8 nút cho chuột
          for (uint8_t i = 0; i < maxBtn; i++)
            bList += String(i) + (i < maxBtn - 1 ? "\n" : "");
          int pick = u8g2.userInterfaceSelectionList("Button Index", kc + 1, bList.c_str());
          if (pick > 0) kc = pick - 1;
        }
        break;
      }
      case 7:
        addThresholdMapping(p, (InputSource)srce, threshOnlyDstCodes[destIdx],
                            pos, neg, abs_, kc, (CombineMode)cmb);
        return;
      default: break;
    }
  }
}

void editMapping(Profile& p) {
  while (true) {
    // build list
    String items = "";
    for (int i = 0; i < p.mappingCount; i++) {
      const Mapping* m = getMapping(p, i);
      items += mappingToString(*m) + "\n";
    }
    items += "[Add]";

    int sel = u8g2.userInterfaceSelectionList("Mappings", 1, items.c_str());
    if (sel == 0) break;

    if (sel == p.mappingCount + 1) {
      // Add new
      const char* addOpts = "Add Axis\nAdd Threshold";
      int opt = u8g2.userInterfaceSelectionList("Add Mapping", 1, addOpts);
      if (opt == 1) axsAdd(p);
      if (opt == 2) thrAdd(p);

    } else {
      // Edit/Delete existing
      int idx = sel - 1;
      const Mapping* m = getMapping(p, idx);

      const char* editOpts = "Edit\nDelete";
      int opt = u8g2.userInterfaceSelectionList(mappingToString(*m).c_str(), 1, editOpts);

      if (opt == 1) {
        // Edit — không đổi loại, chỉ đổi giá trị
        if (m->isAxis) {
          // reuse axsAdd logic nhưng pre-fill giá trị
          uint8_t srce  = m->src;
          uint8_t destIdx = 0;
          // tìm destIdx trong axisOnlyDstCodes
          for (int i = 0; i < axisOnlyDstCount; i++)
            if (axisOnlyDstCodes[i] == m->dst) { destIdx = i; break; }
          float im = m->data.axis.inMin,  ix = m->data.axis.inMax;
          float om = m->data.axis.outMin, ox = m->data.axis.outMax;
          bool  cmp = m->data.axis.clamp;
          uint8_t cmb = m->combine;
          int esel = 1;

          while (esel != 0) {
            String opts =
              "Input:  " + String(srcLabels[srce]) + " [" + String(im,0) + "~" + String(ix,0) + "]\n"
              "Output: " + String(axisOnlyDstLabels[destIdx]) + " [" + String(om,0) + "~" + String(ox,0) + "]\n"
              "Clamp: "  + String(cmp ? "Yes" : "No") + "\n"
              "Combine: ";
            switch (cmb) {
              case COMBINE_NONE: opts += "None"; break;
              case COMBINE_POS:  opts += "+";    break;
              case COMBINE_NEG:  opts += "-";    break;
              default: break;
            }
            opts += "\nConfirm";
            u8g2.setFont(u8g2_font_gulim11_t_korean1);
            esel = u8g2.userInterfaceSelectionList("Edit Axis Map", esel, opts.c_str());
            bool imuSrc = (srce >= SRC_GYRO_X);
            switch (esel) {
              case 1: {
                String sList = "";
                for (int i = 0; i < SRC_COUNT; i++)
                  sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
                int pick = u8g2.userInterfaceSelectionList("Input Source", srce + 1, sList.c_str());
                if (pick > 0) srce = pick - 1;
                break;
              }
              case 2: {
                String dList = "";
                for (int i = 0; i < axisOnlyDstCount; i++)
                  dList += String(axisOnlyDstLabels[i]) + (i < axisOnlyDstCount - 1 ? "\n" : "");
                int pick = u8g2.userInterfaceSelectionList("Output Target", destIdx + 1, dList.c_str());
                if (pick > 0) destIdx = pick - 1;
                break;
              }
              case 3: cmp = !cmp; break;
              case 4: {
                const char* cmbOpts = "None\nAdd (+)\nSubtract (-)";
                int pick = u8g2.userInterfaceSelectionList("Combine", cmb + 1, cmbOpts);
                if (pick > 0) cmb = pick - 1;
                break;
              }
              case 5:
                editAxisMapping(p, idx, (InputSource)srce, axisOnlyDstCodes[destIdx],
                                im, ix, om, ox, cmp, (CombineMode)cmb);
                esel = 0;
                break;
              default: break;
            }
          }

        } else {
          // Threshold edit
          uint8_t srce = m->src;
          uint8_t destIdx = 0;
          for (int i = 0; i < threshOnlyDstCount; i++)
            if (threshOnlyDstCodes[i] == m->dst) { destIdx = i; break; }
          float pos  = m->data.threshold.posThresh;
          float neg  = m->data.threshold.negThresh;
          float abs_ = m->data.threshold.absThresh;
          uint8_t kc = m->keycode;
          int esel = 1;

          while (esel != 0) {
            String opts =
              "Input:   " + String(srcLabels[srce]) + "\n"
              "Target:  " + String(threshOnlyDstLabels[destIdx]) + "\n"
              "Pos thr: " + String(pos, 2) + "\n"
              "Neg thr: " + String(neg, 2) + "\n"
              "Abs thr: " + String(abs_, 2) + "\n"
              "Keycode: " + (destIdx == 0 ? codeToName(kc) : String(kc)) + "\n"
              "Confirm";
            esel = u8g2.userInterfaceSelectionList("Edit Threshold", esel, opts.c_str());
            switch (esel) {
              case 1: {
                String sList = "";
                for (int i = 0; i < SRC_COUNT; i++)
                  sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
                int pick = u8g2.userInterfaceSelectionList("Input Source", srce + 1, sList.c_str());
                if (pick > 0) srce = pick - 1;
                break;
              }
              case 2: {
                String dList = "";
                for (int i = 0; i < threshOnlyDstCount; i++)
                  dList += String(threshOnlyDstLabels[i]) + (i < threshOnlyDstCount - 1 ? "\n" : "");
                int pick = u8g2.userInterfaceSelectionList("Output Target", destIdx + 1, dList.c_str());
                if (pick > 0) destIdx = pick - 1;
                break;
              }
              case 3: pos  = valueSet("Pos Threshold", pos,  false, 0.0f, 500.0f); break;
              case 4: neg  = valueSet("Neg Threshold", neg,  false, 0.0f, 500.0f); break;
              case 5: abs_ = valueSet("Abs Threshold", abs_, false, 0.0f, 500.0f); break;
              case 6: {
                if (destIdx == 0) { // OUT_KEY thì chọn keycode, còn OUT_BTN_GP / OUT_MOUSE_BTN thì chọn button index (cũng lưu trong keycode)
                  int pick = u8g2.userInterfaceSelectionList("Keycode", codeToIndex(kc), buttonName);
                  if (pick > 0) kc = buttonCode[pick - 1];
                } else {
                  String bList = "";
                  uint8_t maxBtn = (threshOnlyDstCodes[destIdx] == OUT_BTN_GP) ? 16 : 8;
                  for (uint8_t i = 0; i < maxBtn; i++)
                    bList += String(i) + (i < maxBtn - 1 ? "\n" : "");
                  int pick = u8g2.userInterfaceSelectionList("Button Index", kc + 1, bList.c_str());
                  if (pick > 0) kc = pick - 1;
                }
                break;
              }
              case 7:
                editThresholdMapping(p, idx, (InputSource)srce, threshOnlyDstCodes[destIdx],
                                     pos, neg, abs_, kc, (CombineMode)COMBINE_NONE);
                esel = 0;
                break;
              default: break;
            }
          }
        }

      } else if (opt == 2) {
        // Delete
        int confirm = u8g2.userInterfaceMessage(
          "Delete mapping?", mappingToString(*m).c_str(), "", " Yes \n No "
        );
        if (confirm == 1) removeMapping(p, idx);
      }
    }
  }
}

void testProfilev2() {
  u8g2.clearBuffer();
  int opt0 = 1;
  const char main_menu[] =
    "Mappings\n"
    "Save Profile\n"
    "Load Profile\n"
    "Delete Profile"
  ;
  while (opt0 > 0) {
    opt0 = u8g2.userInterfaceSelectionList("Profile", opt0, main_menu);
    bool oprtStatus = false;
    switch (opt0) {
    case 1:
      editMapping(testProfile);
      break;
    case 2:
      oprtStatus = saveProfile("/vprofile.cfg", testProfile);
      u8g2.userInterfaceMessage("Save Profile", oprtStatus ? "[pass]" : "[fail]", "", " ok ");
      break;
    case 3:
      oprtStatus = loadProfile("/vprofile.cfg", testProfile);
      u8g2.userInterfaceMessage("Load Profile", oprtStatus ? "[pass]" : "[fail]", "", " ok ");
      break;
    case 4:
      u8g2.userInterfaceMessage("Nothing", "nothing", "nothing", " nothing ");
      break;
    default:
      break;
    }
  }
}

void keypadMUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_mf);
  u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
  u8g2.drawStr(0, 18, "Mode: Keypad");
  switch (inputHandler) {
    case 0:
      u8g2.drawStr(0, 26, "Input: Digital Emulation");
      u8g2.drawStr(0, 34, ("Actuation: " + String(actuation)).c_str());
      break;
    case 1:
      u8g2.drawStr(0, 26, "Input: Hysteresis Handling");
      u8g2.drawStr(0, 34, ("Upper: " + String(upperThreshold)).c_str());
      u8g2.drawStr(0, 42, ("Lower: " + String(lowerThreshold)).c_str());
      break;
    case 2:
      u8g2.drawStr(0, 26, "Input: Dynamic Actuation");
      u8g2.drawStr(0, 34, ("Window Size: " + String(windowSize)).c_str());
      // The foot value is dynamic, so not display it
      break;
    default: u8g2.drawStr(0, 26, "Input: Digital Emulation"); break;
  }
  u8g2.drawStr(0, 40, ("update rate: " + String(lastRate) + "r/s").c_str());
  //float maxPress = 0.00;
  for (int i = 0; i < 3; i++) {
    if (hallVal[i] > 0.05) waitIDLE = millis();
    u8g2.drawFrame(43 * i - 1, 55, 44, 10);
    u8g2.drawBox(43 * i, 55, (int)(hallVal[i] * 43), 10);
    u8g2.setDrawColor(2);
    String line = String(hallVal[i], 2);
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
      if (nowPress[i]) keycodes[idx++] = layout[i];
    tud_hid_keyboard_report(dev.report_id, 0, keycodes);
  }
}

void mouseMUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_mf);
  u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
  u8g2.drawStr(0, 18, "Mode: Mouse");
  u8g2.drawStr(0, 26, ("update rate: " + String(lastRate) + "r/s").c_str());
  u8g2.drawFrame(4, 30, 16, 16); // mouse direction
  u8g2.drawBox(4 + (int)(mOutputs.mouseX * 16 / 127), 30 + (int)(mOutputs.mouseY * 16 / 127), 4, 4);
  u8g2.drawFrame(20, 30, 6, 16); // mouse wheel
  if (mOutputs.mouseWheel != 0) {
    u8g2.drawBox(20, 30 + (mOutputs.mouseWheel > 0 ? 0 : 8), 6, 8);
  }
  u8g2.sendBuffer();
}

void handleMouse() {
  applyMappings(testProfile, mOutputs);
  tud_hid_mouse_report(mdev.report_id, mOutputs.mouseButtons, -mOutputs.mouseX, -mOutputs.mouseY, mOutputs.mouseWheel, 0);
  // mdev.move(mOutputs.mouseX, mOutputs.mouseY);
}

void gamepadMUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_mf);
  u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
  u8g2.drawStr(0, 18, "Mode: Gamepad");
  u8g2.drawStr(0, 26, ("update rate: " + String(lastRate) + "r/s").c_str());
  u8g2.sendBuffer();
}

void handleGamepad() {
  applyMappings(testProfile, mOutputs);
  gdev.sendAll(
    mOutputs.gpButtons,    // buttons (uint32_t)
    mOutputs.axes[0],      // x  = LX
    mOutputs.axes[1],      // y  = LY
    mOutputs.axes[2],      // z  = RX
    mOutputs.axes[3],      // rz = RY
    mOutputs.axes[4],      // rx = LT
    mOutputs.axes[5],      // ry = RT
    0                   // hat (D-pad, không dùng → 0)
  );
}