#include "profile.h"

String configPath = "/default.json";
bool systemReset = false;

bool saveConfig(const char *path) {
  File file = LittleFS.open(path, "w");
  if (!file) return false;

  DynamicJsonDocument doc(1024);

  // array
  doc["cX"][0] = calMax[0];
  doc["cX"][1] = calMax[1];
  doc["cX"][2] = calMax[2];

  doc["cM"][0] = calMin[0];
  doc["cM"][1] = calMin[1];
  doc["cM"][2] = calMin[2];

  for (int i = 0; i < 6; i++) {
    doc["lO"][i] = layout[i];
  }

  for (int i = 0; i < 3; i++) {
    doc["rC"][i] = color[i];
  }

  // int
  doc["dZ"] = deadZone;
  doc["fT"] = filterType;
  doc["iH"] = inputHandler;
  doc["sS"] = screenSaveDuration;
  doc["sO"] = screenOffDuration;
  doc["sB"] = screenBri;
  doc["lg"] = logoType;
  doc["gT"] = glowType;
  doc["rB"] = rgbBri;
  doc["rS"] = rainbowStep;
  doc["rI"] = rgbInterval;

  // float
  doc["aT"] = actuation;
  doc["uT"] = upperThreshold;
  doc["lT"] = lowerThreshold;
  doc["wS"] = windowSize;

  // bool
  doc["dF"] = doFilter;
  doc["uG"] = underGlow;
  doc["rL"] = rgb;
  doc["dR"] = doRainbow;

  // string
  doc["bN"] = btName;
  doc["sL"] = screenLogo;

  if (serializeJsonPretty(doc, file) == 0) {
    file.close();
    return false;
  }
  file.close();
  return true;
}

bool loadConfig(const char *path) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    return false;
  }

  // array float calMax
  if (doc["cX"].is<JsonArray>()) {
    for (int i = 0; i < 3; i++) {
      calMax[i] = doc["cX"][i] | calMax[i];
    }
  }

  if (doc["cM"].is<JsonArray>()) {
    for (int i = 0; i < 3; i++) {
      calMin[i] = doc["cM"][i] | calMin[i];
    }
  }

  if (doc["lO"].is<JsonArray>()) {
    for (int i = 0; i < 6; i++) {
      layout[i] = doc["lO"][i] | layout[i];
    }
  }

  if (doc["rC"].is<JsonArray>()) {
    for (int i = 0; i < 3; i++) {
      color[i] = doc["rC"][i] | color[i];
    }
  }

  // int
  if (doc["rB"].is<int>()) rgbBri = constrain(doc["rB"].as<int>(), 0, 255);
  if (doc["dZ"].is<int>()) deadZone = constrain(doc["dZ"].as<int>(), 0, 4095);
  if (doc["lg"].is<int>()) logoType = constrain(doc["lg"].as<int>(), 0, 12);
  if (doc["gT"].is<int>()) glowType = constrain(doc["gT"].as<int>(), 0, 5);
  if (doc["sB"].is<int>()) screenBri = constrain(doc["sB"].as<int>(), 0, 255);
  if (doc["fT"].is<int>()) filterType = constrain(doc["fT"].as<int>(), 0, 2);
  if (doc["rS"].is<int>()) rainbowStep = doc["rS"].as<int>();
  if (doc["rI"].is<int>()) rgbInterval = doc["rI"].as<int>();
  if (doc["iH"].is<int>()) inputHandler = constrain(doc["iH"].as<int>(), 0, 2);
  if (doc["sO"].is<int>()) screenOffDuration = doc["sO"].as<int>();
  if (doc["sS"].is<int>()) screenSaveDuration = doc["sS"].as<int>();

  // bool
  if (doc["rL"].is<bool>()) rgb = doc["rL"].as<bool>();
  if (doc["dF"].is<bool>()) doFilter = doc["dF"].as<bool>();
  if (doc["uG"].is<bool>()) underGlow = doc["uG"].as<bool>();
  if (doc["dR"].is<bool>()) doRainbow = doc["dR"].as<bool>();

  // float
  if (doc["aT"].is<float>()) {
    float data = doc["aT"].as<float>();
    if (data > 0.0f && data < 1.0f) actuation = data;
  }
  if (doc["wS"].is<float>()) {
    float data = doc["wS"].as<float>();
    if (data > 0.0f && data < 1.0f) windowSize = data;
  }
  if (doc["uT"].is<float>()) {
    float data = doc["uT"].as<float>();
    if (data > 0.0f && data < 1.0f) upperThreshold = data;
  }
  if (doc["lT"].is<float>()) {
    float data = doc["lT"].as<float>();
    if (data > 0.0f && data < 1.0f) lowerThreshold = data;
  }

  // String
  if (doc["bN"].is<const char*>()) {
    const char* data = doc["bN"].as<const char*>();
    if (data) btName = String(data);
  }
  if (doc["sL"].is<const char*>()) {
    const char* data = doc["sL"].as<const char*>();
    if (data) screenLogo = String(data);
  }

  return true;
}

bool sysSave() {
  File file = LittleFS.open("/sys.cfg", "w");
  if (!file) return false;
  DynamicJsonDocument doc(64);
  doc["cp"] = configPath;
  doc["sr"] = systemReset;
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
    sel = u8g2.userInterfaceSelectionList("Profiles", sel, menuItems);
    
    if (sel == 1) {
      std::vector<String> profiles = listProfiles();
      int profilesCount = profiles.size();
      if (profilesCount) {
        String subSel = "";
        for (int i = 0; i < profilesCount; i++) {
          subSel += profiles[i];
          if (i < profilesCount - 1) subSel += "\n";
        }
        int choose = u8g2.userInterfaceSelectionList("Set Default", 0, subSel.c_str());
        if (choose != 0) {
          String prfName = "/" + profiles[choose - 1];
          configPath = prfName;
          if (loadConfig(prfName.c_str()) && sysSave()) {
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
        int choose = u8g2.userInterfaceSelectionList("Load Profile", 0, subSel.c_str());
        if (choose != 0) {
          String prfName = "/" + profiles[choose - 1];
          if (loadConfig(prfName.c_str())) {
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
        int act = u8g2.userInterfaceSelectionList("Save Profile", 0, actions);
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
                if (saveConfig(prfName.c_str())) {
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
            int choose = u8g2.userInterfaceSelectionList("Replace Profile", 0, subSel.c_str());
            if (choose != 0) {
              prfName = "/" + profiles[choose - 1];
              if (saveConfig(prfName.c_str())) {
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
        int choose = u8g2.userInterfaceSelectionList("Delete Profile", 0, subSel.c_str());
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