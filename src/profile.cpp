#include "profile.h"

String configPath = "/default.json";
String mappingPath = "/mapping.json";
bool systemReset = false;

Profile prf;
OutputState mos;

void unpackProfile(Profile& p) {

  // Input
  inputHandler = p.inputHandler;
  actuation = p.actuation;
  windowSize = p.windowSize;
  upperThreshold = p.upperThreshold;
  lowerThreshold = p.lowerThreshold;
  for (int i = 0; i < 3; i++) calMax[i] = p.calMax[i];
  for (int i = 0; i < 3; i++) calMin[i] = p.calMin[i];
  for (int i = 0; i < 3; i++) deadZone[i] = p.deadZone[i];
  doFilter = p.doFilter;
  emaAlpha = p.emaAlpha;
  ovsSamples = p.ovsSamples;
  filterType = p.filterType;
  for (int i = 0; i < 6; i++) layout[i] = p.layout[i];

  // Display
  screenBri = p.screenBri;
  screenSaveDuration = p.screenSaveDuration;
  screenOffDuration = p.screenOffDuration;
  logoType = p.logoType;
  screenLogo = String(p.screenLogo);

  // Effects
  underGlow = p.underGlow;
  glowType = p.glowType;
  rgb = p.rgb;
  rgbBri = p.rgbBri;
  for (int i = 0; i < 3; i++) color[i] = p.color[i];
  doRainbow = p.doRainbow;
  rainbowStep = p.rainbowStep;
  rgbInterval = p.rgbInterval;

  // BLE
  btName = String(p.btName);
}

void packProfile(Profile& p) {

  // Input
  p.inputHandler = inputHandler;
  p.actuation = actuation;
  p.windowSize = windowSize;
  p.upperThreshold = upperThreshold;
  p.lowerThreshold = lowerThreshold;
  for (int i = 0; i < 3; i++) p.calMax[i] = calMax[i];
  for (int i = 0; i < 3; i++) p.calMin[i] = calMin[i];
  for (int i = 0; i < 3; i++) p.deadZone[i] = deadZone[i];
  p.doFilter = doFilter;
  p.filterType = filterType;
  p.emaAlpha = emaAlpha;
  p.ovsSamples = ovsSamples;
  for (int i = 0; i < 6; i++) p.layout[i] = layout[i];

  // Display
  p.screenBri = screenBri;
  p.screenSaveDuration = screenSaveDuration;
  p.screenOffDuration = screenOffDuration;
  p.logoType = logoType;
  strncpy(p.screenLogo, screenLogo.c_str(), sizeof(p.screenLogo) - 1);

  // Effects
  p.underGlow = underGlow;
  p.glowType = glowType;
  p.rgb = rgb;
  p.rgbBri = rgbBri;
  for (int i = 0; i < 3; i++) p.color[i] = color[i];
  p.doRainbow = doRainbow;
  p.rainbowStep = rainbowStep;
  p.rgbInterval = rgbInterval;

  // BLE
  strncpy(p.btName, btName.c_str(), sizeof(p.btName) - 1);
}

bool saveProfile(const char* path, Profile& p) {
  packProfile(p);
  File file = LittleFS.open(path, "w");
  if (!file) return false;

  DynamicJsonDocument doc(3072);

  doc["version"] = 1;

  doc["pt"] = 2;
  doc["um"] = p.usbMode;
  doc["bl"] = p.ble;

  // Input
  doc["ih"] = p.inputHandler;
  doc["at"] = p.actuation;
  doc["ws"] = p.windowSize;
  doc["ut"] = p.upperThreshold;
  doc["lt"] = p.lowerThreshold;
  doc["df"] = p.doFilter;
  doc["ft"] = p.filterType;
  doc["ea"] = p.emaAlpha;
  doc["os"] = p.ovsSamples;
  for (int i = 0; i < 3; i++) doc["dz"][i] = p.deadZone[i];
  for (int i = 0; i < 3; i++) doc["cx"][i] = p.calMax[i];
  for (int i = 0; i < 3; i++) doc["cm"][i] = p.calMin[i];
  for (int i = 0; i < 6; i++) doc["lo"][i] = p.layout[i];

  // Display
  doc["sb"] = p.screenBri;
  doc["ss"] = p.screenSaveDuration;
  doc["so"] = p.screenOffDuration;
  doc["lg"] = p.logoType;
  doc["sl"] = p.screenLogo;

  // Effects
  doc["ug"] = p.underGlow;
  doc["gt"] = p.glowType;
  doc["rl"] = p.rgb;
  doc["rb"] = p.rgbBri;
  for (int i = 0; i < 3; i++) doc["rc"][i] = p.color[i];
  doc["dr"] = p.doRainbow;
  doc["rs"] = p.rainbowStep;
  doc["ri"] = p.rgbInterval;

  // BLE
  doc["bn"] = p.btName;

  // Mapping
  JsonArray maps = doc.createNestedArray("mp");
  for (int i = 0; i < p.mappingCount; i++) {
    const Mapping& m = p.mappings[i];
    JsonObject obj = maps.createNestedObject();
    obj["s"]  = (uint8_t)m.src;
    obj["d"]  = (uint8_t)m.dst;
    obj["ax"] = m.isAxis;
    obj["cb"] = (uint8_t)m.combine;
    obj["kc"] = m.keycode;
    obj["a"]  = m.isAxis ? m.data.axis.inMin       : m.data.threshold.posThresh;
    obj["b"]  = m.isAxis ? m.data.axis.inMax        : m.data.threshold.negThresh;
    obj["c"]  = m.isAxis ? m.data.axis.outMin       : m.data.threshold.absThresh;
    obj["d2"] = m.isAxis ? m.data.axis.outMax       : 0.0f;
    obj["cl"] = m.isAxis ? m.data.axis.clamp        : false;
  }

  bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

bool loadProfile(const char* path, Profile& p) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  DynamicJsonDocument doc(3072);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;
  if (!doc["pt"].is<int>() || doc["pt"].as<int>() != 2) return false;

  p.usbMode = doc["um"] | p.usbMode;
  p.ble     = doc["bl"] | p.ble;

  // Input
  p.inputHandler   = doc["ih"] | p.inputHandler;
  p.actuation      = doc["at"] | p.actuation;
  p.windowSize     = doc["ws"] | p.windowSize;
  p.upperThreshold = doc["ut"] | p.upperThreshold;
  p.lowerThreshold = doc["lt"] | p.lowerThreshold;
  p.doFilter       = doc["df"] | p.doFilter;
  p.filterType     = doc["ft"] | p.filterType;
  p.emaAlpha       = doc["ea"] | p.emaAlpha;
  p.ovsSamples     = doc["os"] | p.ovsSamples;
  if (doc["dz"].is<JsonArray>())
    for (int i = 0; i < 3; i++) p.deadZone[i] = doc["dz"] | p.deadZone[i];
  if (doc["cx"].is<JsonArray>())
    for (int i = 0; i < 3; i++) p.calMax[i] = doc["cx"][i] | p.calMax[i];
  if (doc["cm"].is<JsonArray>())
    for (int i = 0; i < 3; i++) p.calMin[i] = doc["cm"][i] | p.calMin[i];
  if (doc["lo"].is<JsonArray>())
    for (int i = 0; i < 6; i++) p.layout[i] = doc["lo"][i] | p.layout[i];

  // Display
  p.screenBri          = doc["sb"] | p.screenBri;
  p.screenSaveDuration = doc["ss"] | p.screenSaveDuration;
  p.screenOffDuration  = doc["so"] | p.screenOffDuration;
  p.logoType           = doc["lg"] | p.logoType;
  if (doc["sl"].is<const char*>())
    strncpy(p.screenLogo, doc["sl"].as<const char*>(), sizeof(p.screenLogo) - 1);

  // Effects
  p.underGlow  = doc["ug"] | p.underGlow;
  p.glowType   = doc["gt"] | p.glowType;
  p.rgb        = doc["rl"] | p.rgb;
  p.rgbBri     = doc["rb"] | p.rgbBri;
  if (doc["rc"].is<JsonArray>())
    for (int i = 0; i < 3; i++) p.color[i] = doc["rc"][i] | p.color[i];
  p.doRainbow  = doc["dr"] | p.doRainbow;
  p.rainbowStep = doc["rs"] | p.rainbowStep;
  p.rgbInterval = doc["ri"] | p.rgbInterval;

  // BLE
  if (doc["bn"].is<const char*>())
    strncpy(p.btName, doc["bn"].as<const char*>(), sizeof(p.btName) - 1);

  // Mapping
  p.mappingCount = 0;
  if (doc["mp"].is<JsonArray>()) {
    for (JsonObject obj : doc["mp"].as<JsonArray>()) {
      if (p.mappingCount >= MAX_MAPPINGS) break;
      Mapping& m = p.mappings[p.mappingCount++];
      m.src     = (InputSource) (obj["s"]  | 0);
      m.dst     = (OutputTarget)(obj["d"]  | 0);
      m.isAxis  =                obj["ax"] | false;
      m.combine = (CombineMode)  (obj["cb"]| 0);
      m.keycode =                obj["kc"] | 0;
      float a   = obj["a"]  | 0.0f;
      float b   = obj["b"]  | 0.0f;
      float c   = obj["c"]  | 0.0f;
      float d   = obj["d2"] | 0.0f;
      bool  cl  = obj["cl"] | false;
      if (m.isAxis) m.data.axis      = { a, b, c, d, cl };
      else          m.data.threshold = { a, b, c };
    }
  }
  unpackProfile(p);
  return true;
}

bool sysSave() {
  File file = LittleFS.open("/sys.cfg", "w");
  if (!file) return false;
  DynamicJsonDocument doc(64);
  doc["cp"] = configPath;
  doc["mp"] = mappingPath;
  doc["sr"] = systemReset;
  doc["um"] = usbMode;
  doc["bt"] = withBLE;
  doc["pv"] = profileVersion;
  doc["vpid"] = vpidSet;
  doc["hak"] = hallDisplayAsKT;
  doc["kt"] = keyTravel;
  if (serializeJsonPretty(doc, file) == 0) {
    file.close();
    return false;
  }
  file.close();
  return true;
}

bool sysLoad() {
  File file = LittleFS.open("/sys.cfg", "r");
  if (!file) return false;
  DynamicJsonDocument doc(64);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    return false;
  }
  if (doc["cp"].is<const char*>()) {
    const char* data = doc["cp"].as<const char*>();
    if (data) configPath = String(data);
  }
  if (doc["mp"].is<const char*>()) {
    const char* data = doc["mp"].as<const char*>();
    if (data) mappingPath = String(data);
  }
  usbMode = doc["um"] | usbMode;
  withBLE = doc["bt"] | withBLE;
  profileVersion = doc["pv"] | profileVersion;
  vpidSet = doc["vpid"] | vpidSet;
  hallDisplayAsKT = doc["hak"] | hallDisplayAsKT;
  keyTravel = doc["kt"] | keyTravel;
  if (doc["sr"].is<bool>()) systemReset = doc["sr"].as<bool>();
  return true;
}

std::vector<String> listProfiles() {
  std::vector<String> prfList;
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    u8g2.clearBuffer();
    u8g2.drawStr((128 - u8g2.getStrWidth("Error!"))/2, 28, "Error!");
    u8g2.drawStr((128 - u8g2.getStrWidth("Can't open FS"))/2, 40, "Can't open FS");
    u8g2.sendBuffer();
    delay(1000);
    return prfList;
  }

  File file = root.openNextFile();
  while (file) {
    String fname = file.name();
    if (fname.endsWith(".json")) {
      prfList.push_back(fname);
    }
    file = root.openNextFile();
  }
  file.close();
  return prfList;
}

void profileMenu() {
  const char menuItems[] = 
    "Default Profile\n"
    "Load Profile\n"
    "Save Profile\n"
    "Delete Profile"
  ;
  int sel = 1;
  while (sel != 0) {
    sel = noidMenu("Profiles", sel, menuItems);
    
    if (sel == 1) {
      std::vector<String> profiles = listProfiles();
      int profilesCount = profiles.size();
      if (profilesCount) {
        String subSel = "";
        for (int i = 0; i < profilesCount; i++) {
          subSel += profiles[i];
          if (i < profilesCount - 1) subSel += "\n";
        }
        int choose = noidMenu("Set Default", 0, subSel.c_str());
        if (choose != 0) {
          String prfName = "/" + profiles[choose - 1];
          configPath = prfName;
          if (sysSave()) {
            u8g2.userInterfaceMessage("Changed default to", prfName.c_str(), "", " Ok ");
          } else {
            u8g2.userInterfaceMessage("Failed to set", prfName.c_str(), "go back and try again", " Ok ");
          }
        }
      } else {
        u8g2.userInterfaceMessage("Sorry but you haven't", "save any profile yet", "go back and save a profile", " Ok ");
      }
    }

    if (sel == 2) {
      std::vector<String> profiles = listProfiles();
      int profilesCount = profiles.size();
      if (profilesCount) {
        String subSel = "";
        for (int i = 0; i < profilesCount; i++) {
          subSel += profiles[i];
          if (i < profilesCount - 1) subSel += "\n";
        }
        int choose = noidMenu("Load Profile", 0, subSel.c_str());
        if (choose != 0) {
          String prfName = "/" + profiles[choose - 1];
          if (loadProfile(prfName.c_str(), prf)) {
            u8g2.userInterfaceMessage("Profile loaded!", prfName.c_str(), "", " Ok ");
          } else {
            u8g2.userInterfaceMessage("Failed to load", prfName.c_str(), "go back and try again", " Ok ");
          }
        }
      } else {
        u8g2.userInterfaceMessage("Sorry but you haven't", "save any profile yet", "go back and save a profile", " Ok ");
      }
    }

    if (sel == 3) {
      String prfName = "New Profile";
      bool seling = true;
      while (seling) {
        const char actions[] =
          "New File\n"
          "Replace File"
        ;
        int act = noidMenu("Save Profile", 1, actions);
        if (act == 0) break;
        if (act == 1) {
          int opt = -1;
          while (opt != 0) {
            opt = u8g2.userInterfaceMessage(
              "",
              "Save Profile",
              prfName.c_str(),
              " ch.Name \n Save "
            );
            if (opt == 1) prfName = keyboard(prfName);
            if (opt == 2) {
              // remove characters not allowed in filenames
              prfName.replace("/", "-");
              prfName.replace("\\", "-");
              prfName.replace(":", "-");
              prfName.replace("*", "-");
              prfName.replace("?", "-");
              prfName.replace("\"", "-");
              prfName.replace("<", "-");
              prfName.replace(">", "-");
              prfName.replace("|", "-");
              prfName.trim();
              if (prfName.length() == 0) {
                u8g2.clearBuffer();
                u8g2.drawStr((128 - u8g2.getStrWidth("Invalid name!"))/2, 32, "Invalid name!");
                u8g2.sendBuffer();
                delay(1000);
              }
              else {
                if (!prfName.endsWith(".json")) prfName += ".json";
                prfName = "/" + prfName;
                if (saveProfile(prfName.c_str(), prf)) {
                  u8g2.userInterfaceMessage("Profile saved!", prfName.c_str(), "", " Ok ");
                  seling = false;
                  opt = 0;
                } else {
                  u8g2.userInterfaceMessage("Failed to save", prfName.c_str(), "go back and try again", " Ok ");
                }
              }
            }
          }
        }
        if (act == 2) {
          std::vector<String> profiles = listProfiles();
          int profilesCount = profiles.size();
          if (profilesCount) {
            String subSel = "";
            for (int i = 0; i < profilesCount; i++) {
              subSel += profiles[i];
              if (i < profilesCount - 1) subSel += "\n";
            }
            int choose = noidMenu("Replace Profile", 1, subSel.c_str());
            if (choose != 0) {
              prfName = "/" + profiles[choose - 1];
              if (saveProfile(prfName.c_str(), prf)) {
                u8g2.userInterfaceMessage("Profile saved!", prfName.c_str(), "", " Ok ");
                seling = false;
              } else {
                u8g2.userInterfaceMessage("Failed to save", prfName.c_str(), "go back and try again", " Ok ");
              }
            }
          } else {
            u8g2.userInterfaceMessage("Sorry but you haven't", "save any profile yet", "go back and save a profile", " Ok ");
          }
        }
      }
    }

    if (sel == 4) {
      std::vector<String> profiles = listProfiles();
      int profilesCount = profiles.size();
      if (profilesCount) {
        String subSel = "";
        for (int i = 0; i < profilesCount; i++) {
          subSel += profiles[i];
          if (i < profilesCount - 1) subSel += "\n";
        }
        int choose = noidMenu("Delete Profile", 0, subSel.c_str());
        if (choose != 0) {
          String prfName = "/" + profiles[choose - 1];
          int confirm = u8g2.userInterfaceMessage(
            "Are you sure to delete",
            prfName.c_str(),
            "action cannot be undone",
            " Yes \n No "
          );
          if (confirm == 1) {
            if (LittleFS.remove(prfName.c_str())) {
              u8g2.userInterfaceMessage("Profile deleted!", prfName.c_str(), "", " Ok ");
            } else {
              u8g2.userInterfaceMessage("Failed to delete", prfName.c_str(), "go back and try again", " Ok ");
            }
          }
        }
      } else {
        u8g2.userInterfaceMessage("Sorry but you haven't", "save any profile yet", "go back and save a profile", " Ok ");
      }
    }
  }
  l.setBrightness(0);
  l.show();
}

static const char* axisOnlyDstLabels[] = {
  "LX","LY","RX","RY","LT","RT",
  "MsX","MsY","MsW"
};
static const OutputTarget axisOnlyDstCodes[] = {
  OUT_AXIS_LX, OUT_AXIS_LY, OUT_AXIS_RX, OUT_AXIS_RY, OUT_AXIS_LT, OUT_AXIS_RT,
  OUT_MOUSE_X, OUT_MOUSE_Y, OUT_MOUSE_WHEEL
};
static const int axisOnlyDstCount = 9;

static const char* threshOnlyDstLabels[] = {
  "Key","GPBtn","MsBtn"
};
static const OutputTarget threshOnlyDstCodes[] = {
  OUT_KEY, OUT_BTN_GP, OUT_MOUSE_BTN
};
static const int threshOnlyDstCount = 3;

void axsICfg(uint8_t& srce, float& im, float& ix, bool imuSrc) {
  int ssel = 1;
  while (ssel != 0) {
    String sopt =
      "Source: " + String(srcLabels[srce]) + "\n"
      "Min: "    + String(im, 2) + "\n"
      "Max: "    + String(ix, 2);
    ssel = noidMenu("Input", ssel, sopt.c_str());
    switch (ssel) {
      case 1: {
        String sList = "";
        for (int i = 0; i < SRC_COUNT; i++)
          sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
        int pick = noidMenu("Input Source", srce + 1, sList.c_str());
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
}

void axsOCfg(uint8_t& destIdx, float& om, float& ox) {
  int ssel = 1;
  while (ssel != 0) {
    String sopt =
      "Target: " + String(axisOnlyDstLabels[destIdx]) + "\n"
      "Min: "    + String(om, 2) + "\n"
      "Max: "    + String(ox, 2);
    ssel = noidMenu("Output", ssel, sopt.c_str());
    switch (ssel) {
      case 1: {
        String dList = "";
        for (int i = 0; i < axisOnlyDstCount; i++)
          dList += String(axisOnlyDstLabels[i]) + (i < axisOnlyDstCount - 1 ? "\n" : "");
        int pick = noidMenu("Output Target", destIdx + 1, dList.c_str());
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
}

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
    sel = noidMenu("Add Axis Map", sel, opts.c_str());

    switch (sel) {
      case 1: axsICfg(srce, im, ix, imuSrc); break;
      case 2: axsOCfg(destIdx, om, ox); break;
      case 3: cmp = !cmp; break;
      case 4: {
        const char* cmbOpts = "None\nAdd (+)\nSubtract (-)";
        int pick = noidMenu("Combine Mode", cmb + 1, cmbOpts);
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
    sel = noidMenu("Add Threshold", sel, opts.c_str());

    switch (sel) {
      case 1: { // Input source
        String sList = "";
        for (int i = 0; i < SRC_COUNT; i++)
          sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
        int pick = noidMenu("Input Source", srce + 1, sList.c_str());
        if (pick > 0) srce = pick - 1;
        break;
      }
      case 2: { // Target
        String dList = "";
        for (int i = 0; i < threshOnlyDstCount; i++)
          dList += String(threshOnlyDstLabels[i]) + (i < threshOnlyDstCount - 1 ? "\n" : "");
        int pick = noidMenu("Output Target", destIdx + 1, dList.c_str());
        if (pick > 0) destIdx = pick - 1;
        break;
      }
      case 3: pos  = valueSet("Pos Threshold", pos,  false, 0.0f, 500.0f); break;
      case 4: neg  = valueSet("Neg Threshold", neg,  false, 0.0f, 500.0f); break;
      case 5: abs_ = valueSet("Abs Threshold", abs_, false, 0.0f, 500.0f); break;
      case 6: {
        if (destIdx == 0) { // OUT_KEY thì chọn keycode, còn OUT_BTN_GP / OUT_MOUSE_BTN thì chọn button index (cũng lưu trong keycode)
          int pick = noidMenu("Keycode", codeToIndex(kc), buttonName);
          if (pick > 0) kc = buttonCode[pick - 1];
        } else {
          String bList = "";
          uint8_t maxBtn = (threshOnlyDstCodes[destIdx] == OUT_BTN_GP) ? 16 : 8; // tối đa 16 nút cho gamepad, 8 nút cho chuột
          for (uint8_t i = 0; i < maxBtn; i++)
            bList += String(i) + (i < maxBtn - 1 ? "\n" : "");
          int pick = noidMenu("Button Index", kc + 1, bList.c_str());
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

    int sel = noidMenu("Mappings", 1, items.c_str());
    if (sel == 0) break;

    if (sel == p.mappingCount + 1) {
      // Add new
      const char* addOpts = "Add Axis\nAdd Threshold";
      int opt = noidMenu("Add Mapping", 1, addOpts);
      if (opt == 1) axsAdd(p);
      if (opt == 2) thrAdd(p);

    } else {
      // Edit/Delete existing
      int idx = sel - 1;
      const Mapping* m = getMapping(p, idx);

      const char* editOpts = "Edit\nDelete";
      int opt = noidMenu(mappingToString(*m).c_str(), 1, editOpts);

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
            esel = noidMenu("Edit Axis Map", esel, opts.c_str());
            bool imuSrc = (srce >= SRC_GYRO_X);
            switch (esel) {
              case 1: axsICfg(srce, im, ix, imuSrc); break;
              case 2: axsOCfg(destIdx, om, ox); break;
              case 3: cmp = !cmp; break;
              case 4: {
                const char* cmbOpts = "None\nAdd (+)\nSubtract (-)";
                int pick = noidMenu("Combine", cmb + 1, cmbOpts);
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
            esel = noidMenu("Edit Threshold", esel, opts.c_str());
            switch (esel) {
              case 1: {
                String sList = "";
                for (int i = 0; i < SRC_COUNT; i++)
                  sList += String(srcLabels[i]) + (i < SRC_COUNT - 1 ? "\n" : "");
                int pick = noidMenu("Input Source", srce + 1, sList.c_str());
                if (pick > 0) srce = pick - 1;
                break;
              }
              case 2: {
                String dList = "";
                for (int i = 0; i < threshOnlyDstCount; i++)
                  dList += String(threshOnlyDstLabels[i]) + (i < threshOnlyDstCount - 1 ? "\n" : "");
                int pick = noidMenu("Output Target", destIdx + 1, dList.c_str());
                if (pick > 0) destIdx = pick - 1;
                break;
              }
              case 3: pos  = valueSet("Pos Threshold", pos,  false, 0.0f, 500.0f); break;
              case 4: neg  = valueSet("Neg Threshold", neg,  false, 0.0f, 500.0f); break;
              case 5: abs_ = valueSet("Abs Threshold", abs_, false, 0.0f, 500.0f); break;
              case 6: {
                if (destIdx == 0) { // OUT_KEY thì chọn keycode, còn OUT_BTN_GP / OUT_MOUSE_BTN thì chọn button index (cũng lưu trong keycode)
                  int pick = noidMenu("Keycode", codeToIndex(kc), buttonName);
                  if (pick > 0) kc = buttonCode[pick - 1];
                } else {
                  String bList = "";
                  uint8_t maxBtn = (threshOnlyDstCodes[destIdx] == OUT_BTN_GP) ? 16 : 8;
                  for (uint8_t i = 0; i < maxBtn; i++)
                    bList += String(i) + (i < maxBtn - 1 ? "\n" : "");
                  int pick = noidMenu("Button Index", kc + 1, bList.c_str());
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