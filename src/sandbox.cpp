#include "sandbox.h"

Profile testProfile;

float valueSet(const char *title, float input, bool clamp, float clampMin, float clampMax) {
  #define SMTH_WAIT 150
  #define SMTH_FACC 1500
  #define SMTH_FAHH 10000
  unsigned long htA = 0, htB = 0, adw = 0;
  bool hlA = false, hlB = false;
  float sVal = input;
  while (!digitalRead(btnPins[2])) delay(10);
  while (true) {
    if (digitalRead(btnPins[0])) hlA = false;
    if (digitalRead(btnPins[2])) hlB = false;
    if (!digitalRead(btnPins[0]) && !digitalRead(btnPins[2])) {}
    else if (!digitalRead(btnPins[0])) {
      if (!hlA) {htA = millis(); hlA = true;}
      if (millis() - adw > SMTH_WAIT) {
        adw = millis() + ((millis() - htA < SMTH_FACC) ? 0 : SMTH_WAIT);
        input -= (millis() - htA < SMTH_FAHH ? 0.01 : 0.81);
      }
    }
    else if (!digitalRead(btnPins[2])) {
      if (!hlB) {htB = millis(); hlB = true;}
      if (millis() - adw > SMTH_WAIT) {
        adw = millis() + ((millis() - htB < SMTH_FACC) ? 0 : SMTH_WAIT);
        input += (millis() - htB < SMTH_FAHH ? 0.01 : 0.81);
      }
    }
    if (clamp) input = constrain(input, clampMin, clampMax);
    if (!digitalRead(btnPins[1])) {while (!digitalRead(btnPins[1])) delay(10); return input;}
    if (!digitalRead(btnPins[3])) {while (!digitalRead(btnPins[3])) delay(10); return sVal;}

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 12, title);
    String tmp = String(input, 2);
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

void thrAdd() {

}

void axsAdd(Profile& p) {
  uint8_t srce = 0, dest = 0;
  float im = 0.00, ix = 1.00, om = 0.00, ox = 1.00;
  bool cmp = true;
  uint8_t cmb = COMBINE_NONE;
  int sel = 1;
  while (sel != 0) {
    String opts = "Input\nOutput\nClamp: " + String(cmp ? "Yes" : "No") + "\nCombine: " + "Add Mapping";
    switch (cmb) {
    case 0: opts += "None"; break;
    case 1: opts += "Add"; break;
    case 2: opts += "Subtract"; break;
    default: opts += "None"; break;}
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("Add Axis", sel, opts.c_str());
    switch (sel) {
    case 0: sel = 0; break;
    case 1: {
      int ssel = 1;
      while (ssel != 0) {
        String sopt = "Source: " + String(srcLabels[srce]) + "\n"
        + "Min Value" + String(im, 2) + '\n'
        + "Max Value" + String(ix, 2);
        u8g2.setFont(u8g2_font_gulim11_t_korean1);
        ssel = u8g2.userInterfaceSelectionList("Input", ssel, sopt.c_str());
        switch (ssel) {
          case 1: im = valueSet("Min Input Value", im); break;
          default: break;
        }
      }
    }
    }
  }
}

void editMapping(Profile& p) {
  while (true) {
    String items = "";
    for (int i = 0; i < p.mappingCount; i++) {
      const Mapping* m = getMapping(p, i);
      items += mappingToString(*m);
      items += "\n";
    }
    items += "[add]";
    int sel = u8g2.userInterfaceSelectionList("Mappings", 1, items.c_str());
    if (sel == 0) break;
    if (sel == p.mappingCount + 1) {
      const char opts[] =
      "Add Axis\n"
      "Add Threshold";
      int opt = u8g2.userInterfaceSelectionList("Add new mapping", 1, opts);
      switch (opt) {
      case 1: axsAdd(p); break;
      case 2: thrAdd(); break;
      default: break;
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