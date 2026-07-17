#include "global.h"
#include "input.h"
#include "effect.h"
#include "profile.h"
#include "menu.h"
#include "bledev.h"
#include "cdc.h"
#include "visplayer.h"
#include "macro.h"

bool lastWifiState = false;
int AODcorner = 0;

int min(int a, int b) {
  return (a < b) ? a : b;
}

int max(int a, int b) {
  return (a > b) ? a : b;
}

void mainMenu() {
  const char kmenu_items[] =
    "Hall Settings\n"
    "Layout & Macro\n"
    "Effects\n"
    "Connection\n"
    "Profile\n"
    "System\n"
    "About";
  const char gmenu_items[] =
    "Hall Settings\n"
    "Mapping\n"
    "Effects\n"
    "Connection\n"
    "Profile\n"
    "System\n"
    "About";
  if (analogLed) {
    for (int i = 0; i < 3; i++) ledcWrite(i, 0);
  } else {
    b.fill(b.Color(0, 0, 0));
    b.show();
  }
  l.setBrightness(0);
  l.show();
  u8g2.clearBuffer();
  u8g2.setPowerSave(0);
  u8g2.setFont(u8g2_font_spleen16x32_mr);
  u8g2.drawStr((128 - u8g2.getStrWidth("Mufuki"))/2, 40, "Mufuki");
  globFont();
  u8g2.drawStr((128 - u8g2.getStrWidth("Main Menu"))/2, 54, "Main Menu");
  u8g2.sendBuffer();
  while (!digitalRead(btnPins[3])) delay(10);
  int sel = 1;
  while (sel > 0) {
    globFont();
    sel = noidMenu("Main Menu", sel, usbMode == 0 ? kmenu_items : gmenu_items);
    switch (sel) {
      case 1: {
        int hSel = 1;
        const char hallSItem[] =
        "Calibrate\n"
        "Filter\n"
        "Input Handling\n"
        "Dead Zone";
        while (hSel > 0) {
          globFont();
          hSel = noidMenu("Hall Settings", hSel, hallSItem);
          switch (hSel) {
            case 1: calibMenu(); break;
            case 2: filtMenu(); break;
            case 3: inputMenu(); break;
            case 4: deadCalib(); break;
            default: break;
          }
        }
      } break;
      case 2: {
        if (usbMode == 0) {
          int sMenu = 1;
          while (sMenu > 0) {
            sMenu = noidMenu("Layout & Macro", sMenu, "Layout\nMacro");
            switch (sMenu) {
              case 1: layoutChangeMenu(); break;
              case 2: macroMenu();
              default: break;
            }
          }
        }
        else editMapping(prf);
        break;
      }
      case 3: effectMenu(); break;
      case 4: connectMenu(); break;
      case 5: profileMenu(); break;
      case 6: systemMenu(); break;
      case 7: about(); break;
      default: break;
    }
  }
  // for (int i = 0; i < 3; i++) {
  //   ledcWrite(i, 0);
  //   applyEffect[i] = 0;
  // }
  if (prf.rgb) {
    l.setBrightness(prf.rgbBri);
    l.fill(l.Color(prf.color[0], prf.color[1], prf.color[2]));
    l.show();
  } else {
    l.setBrightness(0);
    l.show();
  }
}

class CSCDCCallbacks : public CDCCallbacks {
  bool onConnect(bool dtr, bool rts) {
    return false;
  }
  void onData() {}
};

void hidTask(void* param) {
  while (true) {
    if (tud_ready() && !menuOpen) {
      switch (usbMode) {
        case 0: handleKeypad(); break;
        case 1: handleGamepad(); break;
        case 2: handleMouse(); break;
      }
    }
    vTaskDelay(1);
  }
}

void displayTask(void* param) {
  l.fill(l.Color(0, 255, 0));
  l.show();
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  waitIDLE = millis();
  while (true) {
    // CDC 100Hz
    static unsigned long lastCDC = 0;
    if (micros() - lastCDC >= 10000) {
      handleCDC(); lastCDC = micros();
      if (usbMode != 0) mpu.update();
    }

    for (int i = 0; i < 3; i++) {
      int mcs = prf.launchMacro[i];
      if (mcs >= 0 && getButton(true) == i) {
        menuOpen = true;
        u8g2.setPowerSave(0);
        screenSaver(("Running Macro " + String(mcs + 1)).c_str());
        if (macQuick[mcs].rep == -1) {
          while (getButton(true) > 0)
            executeMacro(macQuick[mcs], i);
        } else if (macQuick[mcs].rep == 0) executeMacro(macQuick[mcs], i);
        else for (int n = 0; n < macQuick[mcs].rep; n++)
          executeMacro(macQuick[mcs], i);
        menuOpen = false;
        waitIDLE = millis();
        screenWait = false;
        screenOff = false;
      }
    }

    // F4 Key (Menu)
    if (digitalRead(btnPins[3]) == LOW) {
      if (!bt4Hold) {
        bt4Hold = true;
        bt4time = millis();
      }
      if (millis() - bt4time > 500 && bt4Hold) {
        menuOpen = true;
        mainMenu();
        waitIDLE = millis();
        fromMenu = true;
        menuOpen = false;
        if (WiFi.getMode() != WIFI_OFF) lastWifiState = true;
        if (lastWifiState && !timeUpdated) WiFi.mode(WIFI_STA);
      }
      if (screenOff) u8g2.setPowerSave(0);
      screenWait = false;
      screenOff = false;
    } else {
      if (bt4Hold && !fromMenu) {
        if (millis() - waitIDLE < 5000) waitIDLE = millis() - 5000;
        else {
          u8g2.setPowerSave(0);
          waitIDLE = millis();
          screenWait = false;
          screenOff = false;
        }
      }
      if (bt4Hold && 300 < millis() - bt4time && millis() - bt4time < 500 && usbMode != 0) {
        u8g2.clearBuffer();
        const char *tmp = "Calibrating...";
        u8g2.drawStr(64 - u8g2.getStrWidth(tmp) / 2, 32 - u8g2.getMaxCharHeight() / 2, tmp);
        u8g2.sendBuffer();
        mpu.calcOffsets();
      } 
      if (usbMode == 0 && 300 < millis() - bt4time && millis() - bt4time < 500 && bt4Hold && tud_ready()) {
        u8g2.setPowerSave(0);
        menuOpen = true;
        String text = keyboard("");
        int lineCount = 0;
        u8g2.clearBuffer();
        if (text != "") {
          for (int i = 0; i < text.length(); i++) {
            bool shift;
            uint8_t key = charToKey((char)text[i], shift);
            if (key) {
              // KeyReport report = {0};
              // report.modifiers = shift ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
              // report.keys[0] = key;
              uint8_t keycodes[6] = {key, 0, 0, 0, 0, 0};
              uint8_t modifier = 0;
              // if (withBLE) kblue->sendReport(&report); else tud_hid_keyboard_report(dev.report_id, report.modifiers, keycodes);
              tud_hid_keyboard_report(dev.report_id, modifier, keycodes);
              delay(randRange(10, 80));
              // report.modifiers = 0;
              // report.keys[0] = 0;
              modifier = 0;
              keycodes[0] = 0;
              // if (withBLE) kblue->sendReport(&report); else tud_hid_keyboard_report(dev.report_id, report.modifiers, keycodes);
              tud_hid_keyboard_report(dev.report_id, modifier, keycodes);
              String tmpText = String(i) + ". char: "
              + String(char(text[i])) + ", code: 0x" + String(key, HEX);
              u8g2.setDrawColor(0);
              u8g2.drawBox(0, lineCount * 10, 128, 10);
              u8g2.setDrawColor(1);
              u8g2.drawStr(0, 10 + lineCount * 10, tmpText.c_str());
              u8g2.sendBuffer();
              lineCount = (lineCount + 1) % 6;
              delay(randRange(10, 80));
            }
          }
        }
        screenWait = false;
        screenOff = false;
        menuOpen = false;
        waitIDLE = millis();
      }
      fromMenu = false;
      bt4Hold = false;
    }

    // Effects
    if (prf.backlight) {
      switch (prf.glowType) {
        case 0:
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
        break;
        case 1:
          for (int i = 0; i < 3; i++) {
            ledOutput[i] = 0;
            if (applyEffect[i]) {
              addRippleWave(i);
              applyEffect[i] = false;
            }
          }
          updateRipple();
          underGlowUpdate();
        break;
        case 2:
          udgSmooth(); // handled in its own function
        break;
        case 3:
          udgBurnIn(); // also this
        break;
        case 4:
          udgAnalog();
        break;
        case 5:
          udgSoild();
        break;
        default:
        break;
      }
    }

    if (prf.rgb && prf.doRainbow) {
      if (millis() - lastRGBUpdate > prf.rgbInterval * 10) {
        lastRGBUpdate = millis();
        static uint16_t hue = 0;
        hue += 128 + prf.rainbowStep;
        l.rainbow(hue, 1, 255, prf.rgbBri, true);
        l.show();
      }
    }

    // Screen
    static int lastMinute = -1;
    if (millis() - waitIDLE < prf.screenSaveDuration) {
      if (visIsPlaying()) visPlay();
      else {
        switch (usbMode) {
        case 0: keypadMUI(); break;
        case 1: gamepadMUI(); break;
        case 2: mouseMUI(); break;
        default: keypadMUI(); break;
        }
      }
    }
    else if (millis() - waitIDLE < prf.screenSaveDuration + prf.screenOffDuration) {
      if (prf.AOD) {
        lastMinute = -1;
        char timeStr[10];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        u8g2.setFont(u8g2_font_spleen16x32_mr);
        u8g2.clearBuffer();
        drawScrambleText(64 - u8g2.getStrWidth(timeStr) / 2, 32, timeStr);
        char dateStr[12];
        strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);
        u8g2.setFont(u8g2_font_gulim_11_idk);
        u8g2.drawStr(64 - u8g2.getStrWidth(dateStr) / 2, 48, dateStr);
        u8g2.sendBuffer();
      }
      else if (!screenWait) {
        screenSaver("NoID");
        screenWait = true;
      }
    }
    else {
      if (prf.AOD) {
        static unsigned long lastAODUpdate = 0;
        if (timeinfo.tm_min != lastMinute) {
          lastMinute = timeinfo.tm_min;
          char timeStr[10];
          strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
          u8g2.setFont(u8g2_font_spleen16x32_mr);
          u8g2.clearBuffer();
          switch (AODcorner) {
            case 0: u8g2.drawStr(64 - u8g2.getStrWidth(timeStr) / 2, 32 + u8g2.getMaxCharHeight() / 2, timeStr); break;
            case 1: u8g2.drawStr(0, u8g2.getMaxCharHeight(), timeStr); break;
            case 2: u8g2.drawStr(128 - u8g2.getStrWidth(timeStr), u8g2.getMaxCharHeight(), timeStr); break;
            case 3: u8g2.drawStr(0, 64, timeStr); break;
            case 4: u8g2.drawStr(128 - u8g2.getStrWidth(timeStr), 64, timeStr); break;
          }
          u8g2.sendBuffer();
          lastAODUpdate = millis();
          AODcorner = (AODcorner + 1) % 5;
        }
      }
      else if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
      }
    }
    vTaskDelay(1);
  }
}

void timeTask(void* param) {
  unsigned long lastClockUpdate = 0;
  while (true) {
    if (millis() - lastClockUpdate > 5000 && prf.AOD) {
      if (!timeUpdated && WiFi.getMode() != WIFI_OFF && WiFi.status() == WL_CONNECTED) {
        configTime(prf.GMTPlus * 3600, 0, "time.google.com", "time.nist.gov");
        if (getLocalTime(&timeinfo)) {
          timeUpdated = true;
          if (!lastWifiState) WiFi.mode(WIFI_OFF);
        }
      }
      lastClockUpdate = millis();
    }
    if (timeUpdated) getLocalTime(&timeinfo);
    vTaskDelay(1000);
  }
}

void setup() {

  // Wireless
  // BLEDevice::deinit(true);

  // WiFi.mode(WIFI_OFF);

  // Hardware setup
  l.begin();
  b.begin();
  b.setBrightness(maxBri);
  l.fill(l.Color(255, 0, 0));
  l.show();
  Wire.begin(8, 9);
  u8g2.begin(btnPins[1], btnPins[2], btnPins[0], btnPins[0], btnPins[2], btnPins[3]);
  u8g2.setContrast(prf.screenBri);
  u8g2.setFontMode(1);
  u8g2.enableUTF8Print();
  u8g2.setFontRefHeightAll();
  u8g2.setFont(u8g2_font_spleen32x64_mr);
  const char *NoID = "NoID";
  u8g2.drawStr((128 - u8g2.getStrWidth(NoID))/2, 56, NoID);
  u8g2.sendBuffer();
  // u8g2.setFont(u8g2_font_5x8_mf);
  // u8g2.drawStr(0, 8, String(esp_reset_reason()).c_str());
  // u8g2.sendBuffer();
  globFont();
  l.fill(l.Color(255, 255, 0));
  l.show();
  byte status = mpu.begin();
  l.fill(status == 0 ? l.Color(0, 255, 255) : l.Color(255, 0, 0));
  l.show();
  if (status != 0) delay(1000);
  analogSetAttenuation(ADC_11db);

  // PinMode
  for (int i = 0; i < 3; i++) pinMode(adcPins[i], INPUT);
  if (analogLed) for (int i = 0; i < 3; i++) pinMode(ledPins[i], OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(btnPins[i], INPUT_PULLUP);

  // LEDs
  if (analogLed) {
    for (int i = 0; i < 3; i++) {
      ledcSetup(i, 5000, 8);
      ledcAttachPin(ledPins[i], i);
      ledcWrite(i, 0);
    }
  }
  l.fill(l.Color(0, 255, 0));
  l.setBrightness(0);
  l.show();

  // File System
  if (!LittleFS.begin(true)) {
    l.fill(l.Color(255, 0, 0));
    l.setBrightness(255);
    l.show();
    u8g2.userInterfaceMessage("!!WARNING!!", "Fs Mount Failed", "Profile can't load", " OK ");
  } else {
    if (!sysLoad()) {
      firstTime = true;
      firstTimeSetup(); // definitely first time
      firstTime = false;
    }
    if (!loadProfile(configPath.c_str(), prf)) {
      saveProfile(configPath.c_str(), prf); // fallback to default
    }
  }
  if (withBLE) {
    /*
    switch (usbMode) {
      case 0: {
        kblue = new BleKeyboard(btName.c_str(), "NoID", 100);
        kblue->begin();
      } break;
      case 1: {
        gblue = new BleGamepad(btName.c_str(), "NoID", 100);
        BleGamepadConfiguration cfg;
        cfg.setAutoReport(false);
        cfg.setAxesMax(127);
        cfg.setAxesMin(-127);
        cfg.setVid(vpidPair[vpidSet][0]);
        cfg.setPid(vpidPair[vpidSet][1]);
        gblue->begin(&cfg);
        break;
      }
      case 2: {
        mlue = new BleMouse(btName.c_str(), "NoID", 100);
        mlue->begin();
      } break;
      default: {
        kblue = new BleKeyboard(btName.c_str(), "NoID", 100);
        kblue->begin();
      } break;
    }
    */
    u8g2.userInterfaceMessage("Sorry", "BLE is removed", "for now", " ok ");
  } else {
    dev.setBaseEP(3);
    gdev.setBaseEP(3);
    mdev.setBaseEP(3);
    if (usbMode == 1) gdev.deviceID(vpidPair[vpidSet][0], vpidPair[vpidSet][1]);
    switch (usbMode) {
      case 0: dev.begin(); break;
      case 1: gdev.begin(); break;
      case 2: mdev.begin(); break;
      default: dev.begin(); break;
    }
    CDCUSBSerial.begin();
    CDCUSBSerial.setCallbacks(new CSCDCCallbacks());
  }
  waitIDLE = millis();
    if (prf.rgb) {
    l.setBrightness(prf.rgbBri);
    l.fill(l.Color(prf.color[0], prf.color[1], prf.color[2]));
    l.show();
  } else {
    l.setBrightness(0);
    l.show();
  }
  mpu.calcOffsets();
  u8g2.setContrast(prf.screenBri);
  xTaskCreatePinnedToCore(hidTask,     "HID",     4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(displayTask, "Display", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(timeTask,    "Time",    4096, NULL, 1, NULL, 1);
  // vTaskDelete(NULL);
  if (prf.AOD) {
    WiFi.mode(WIFI_STA);
    configTime(prf.GMTPlus * 3600, 0, "pool.ntp.org", "time.google.com");
    if (getLocalTime(&timeinfo)) {timeUpdated = true; WiFi.mode(WIFI_OFF);}
  } else WiFi.mode(WIFI_OFF);
}

void loop() {
  if (micros() - lastLoopTime <= LOOP_INTERVAL_US) return;
  lastLoopTime = micros();
  updateInput();
  rate++;
  if (millis() - lastRateCheckUpdate > 5000) {
    lastRateCheckUpdate = millis();
    lastRate = rate / 5;
    rate = 0;
  }
}