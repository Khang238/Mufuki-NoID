#include "global.h"
#include "input.h"
#include "effect.h"
#include "profile.h"
#include "menu.h"
#include "bledev.h"
#include "cdc.h"
#include "visplayer.h"

int min(int a, int b) {
  return (a < b) ? a : b;
}

int max(int a, int b) {
  return (a > b) ? a : b;
}

void otaUpdate() {
  static uint8_t spinnerIndex = 0;
  static bool doingOTA = true;
  const char spinnerFrames[4] = {'|', '/', '-', '\\'};
  unsigned long displayTime = millis();
  l.setBrightness(128);

  ArduinoOTA
    .onProgress([&](unsigned int progress, unsigned int total) {
      int percent = (progress * 100) / total;

      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_gulim11_t_korean1);

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
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      u8g2.drawStr((128 - u8g2.getStrWidth("Restarting..."))/2, 54, "Restarting...");
      u8g2.sendBuffer();
      l.fill(l.Color(255, 255, 255));
      l.show();
      forceReset();
      doingOTA = false;
    });

  ArduinoOTA.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
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

void mainMenu() {
  const char menu_items[] =
    "Calibration\n"
    "Input handling\n"
    "Filter\n"
    "Mapping\n"
    "Connection\n"
    "Effects\n"
    "System\n"
    "OTA Update\n"
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
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr((128 - u8g2.getStrWidth("Main Menu"))/2, 54, "Main Menu");
  u8g2.sendBuffer();
  while (!digitalRead(btnPins[3])) delay(10);
  int sel = 1;
  while (sel > 0) {
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    sel = u8g2.userInterfaceSelectionList("Main Menu", sel, menu_items);
    switch (sel) {
      case 1: calibMenu(); break;
      case 2: inputMenu(); break;
      case 3: filtMenu(); break;
      case 4: editMapping(prf); break;
      case 5: connectMenu(); break;
      case 6: effectMenu(); break;
      case 7: systemMenu(); break;
      case 8:
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
          if (WiFi.getMode() == WIFI_OFF || WiFi.status() != WL_CONNECTED) break;
        }
        otaUpdate();
        break;
      case 9: about(); break;
      default:
        break;
    }
  }
  for (int i = 0; i < 3; i++) {
    ledcWrite(i, 0);
    applyEffect[i] = 0;
  }
  if (rgb) {
    l.setBrightness(rgbBri);
    l.fill(l.Color(color[0], color[1], color[2]));
    l.show();
  } else {
    l.setBrightness(0);
    l.show();
  }
}

class CSCDCCallbacks : public CDCCallbacks {
  bool onConnect(bool dtr, bool rts) {
    return false;  // không cho phép reset
  }
  void onData() {}
};

/*
void hidTask(void* param) {
  while (true) {
    if (micros() - lastLoopTime >= LOOP_INTERVAL_US) {
      lastLoopTime += LOOP_INTERVAL_US;
      updateInput();
      if (usbMode != 0) mpu.update();
      if (tud_ready() && !menuOpen) {
        switch (usbMode) {
          case 0: handleKeypad(); break;
          case 1: handleGamepad(); break;
          case 2: handleMouse(); break;
        }
      }
      rate++;
      if (millis() - lastRateCheckUpdate > 1000) {
        lastRateCheckUpdate = millis();
        lastRate = rate;
        rate = 0;
      }
    }// else {
     // delayMicroseconds(LOOP_INTERVAL_US - (micros() - lastLoopTime));
     //}
    // vTaskDelay(1); // yield, không block
    taskYIELD();
  }
}
*/

void hidTask(void* param) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t loopPeriod = pdMS_TO_TICKS(1);  // ~1000Hz

  while (true) {
    updateInput();
    if (usbMode != 0) mpu.update();

    if (tud_ready() && !menuOpen) {
      switch (usbMode) {
        case 0: handleKeypad(); break;
        case 1: handleGamepad(); break;
        case 2: handleMouse(); break;
      }
    }
    rate++;
    if (millis() - lastRateCheckUpdate > 1000) {
      lastRateCheckUpdate = millis();
      lastRate = rate;
      rate = 0;
    }
    // Yield + feed watchdog tốt
    vTaskDelayUntil(&lastWakeTime, loopPeriod);
    // hoặc: taskYIELD();
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
    }

    // F4 Key (Menu)
    if (digitalRead(btnPins[3]) == LOW) {
      if (!bt4Hold) {
        bt4Hold = true;
        bt4time = millis();
      }
      if (millis() - bt4time > 1000 && bt4Hold) {menuOpen = true; mainMenu(); waitIDLE = millis(); fromMenu = true; menuOpen = false;}
      if (screenOff) u8g2.setPowerSave(0);
      screenWait = false;
      screenOff = false;
    } else {
      if (bt4Hold && !fromMenu) {
        if (millis() - waitIDLE < 5000) waitIDLE = millis() - 5000;
        else {
          waitIDLE = millis();
          u8g2.setPowerSave(0);
          screenWait = false;
          screenOff = false;
        }
      }
      if (250 < millis() - bt4time && millis() - bt4time < 500 && bt4Hold) {
        u8g2.clearBuffer();
        const char *tmp = "Calibrating...";
        u8g2.drawStr(64 - u8g2.getStrWidth(tmp) / 2, 32 - u8g2.getMaxCharHeight() / 2, tmp);
        u8g2.sendBuffer();
        mpu.calcOffsets();
      }
      fromMenu = false;
      bt4Hold = false;
    }

    // Effects
    if (underGlow) {
      switch (glowType) {
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

    if (rgb && doRainbow) {
      if (millis() - lastRGBUpdate > rgbInterval * 10) {
        lastRGBUpdate = millis();
        static uint16_t hue = 0;
        hue += 128 + rainbowStep;
        l.rainbow(hue, 1, 255, rgbBri, true);
        l.show();
      }
    }

    // Screen
    if (millis() - waitIDLE < screenSaveDuration) {
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
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("NoID");
        screenWait = true;
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
      }
    }
    vTaskDelay(1); // 1ms yield
  }
}

void setup() {

  // Wireless
  BLEDevice::deinit(true);
  WiFi.mode(WIFI_OFF);
  // WiFi.begin("Vi MUNG"); // if this fail i might fucked up

  // Hardware setup
  l.begin();
  b.begin();
  b.setBrightness(maxBri);
  l.fill(l.Color(255, 0, 0));
  l.show();
  Wire.begin(8, 9);
  u8g2.begin(btnPins[1], btnPins[2], btnPins[0], btnPins[0], btnPins[2], btnPins[3]);
  u8g2.setContrast(screenBri);
  u8g2.setFontMode(1);
  u8g2.enableUTF8Print();
  u8g2.setFontRefHeightAll();
  u8g2.setFont(u8g2_font_spleen32x64_mr);
  const char *NoID = "NoID";
  u8g2.drawStr((128 - u8g2.getStrWidth(NoID))/2, 56, NoID);
  u8g2.sendBuffer();
  u8g2.setFont(u8g2_font_5x8_mf);
  u8g2.drawStr(0, 8, String(esp_reset_reason()).c_str());
  u8g2.sendBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
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
  }
  if (!sysLoad()) {
    firstTimeSetup(); // definitely first time
  }
  if (!loadProfile(configPath.c_str(), prf)) {
    saveProfile(configPath.c_str(), prf); // fallback to default
  }
  dev.setBaseEP(3);
  gdev.setBaseEP(3);
  if (usbMode == 1) gdev.deviceID(vpidPair[vpidSet][0], vpidPair[vpidSet][1]);
  switch (usbMode) {
    case 0: dev.begin(); break;
  mdev.setBaseEP(3);
    case 1: gdev.begin(); break;
    case 2: mdev.begin(); break;
    default: dev.begin(); break;
  }
  CDCUSBSerial.begin();
  CDCUSBSerial.setCallbacks(new CSCDCCallbacks());
  waitIDLE = millis();
    if (rgb) {
    l.setBrightness(rgbBri);
    l.fill(l.Color(color[0], color[1], color[2]));
    l.show();
  } else {
    l.setBrightness(0);
    l.show();
  }
  mpu.calcOffsets();
  u8g2.setContrast(screenBri);
  // xTaskCreatePinnedToCore(hidTask,     "HID",     4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(displayTask, "Display", 8192, NULL, 2, NULL, 1);
  // vTaskDelete(NULL);
}

void loop() {
  if (micros() - lastLoopTime <= LOOP_INTERVAL_US) return;
  lastLoopTime = micros();
  updateInput();
  if (usbMode != 0) mpu.update();

  if (tud_ready() && !menuOpen) {
    switch (usbMode) {
      case 0: handleKeypad(); break;
      case 1: handleGamepad(); break;
      case 2: handleMouse(); break;
    }
  }
  rate++;
  if (millis() - lastRateCheckUpdate > 5000) {
    lastRateCheckUpdate = millis();
    lastRate = rate / 5;
    rate = 0;
  }
}

/*
void loop() {

  if (micros() - lastLoopTime < LOOP_INTERVAL_US) return;
  lastLoopTime += LOOP_INTERVAL_US;

  // updateInput();
  if (usbMode != 0) mpu.update();
  
  if (tud_ready()) {
    switch (usbMode) {
      case 0: handleKeypad(); break;
      case 1: handleGamepad(); break;
      case 2: handleMouse(); break;
      default: handleKeypad(); break;
    }
  }

  static unsigned long lastCDCTime = 0;
  unsigned long now = micros();
  if (now - lastCDCTime >= 100000) { // 100Hz, tách khỏi HID 1000Hz
    handleCDC();
    lastCDCTime = now;
  }

  // F4 Key (Menu)
  if (digitalRead(btnPins[3]) == LOW) {
    if (!bt4Hold) {
      bt4Hold = true;
      bt4time = millis();
    }
    if (millis() - bt4time > 250 && bt4Hold) mpu.calcOffsets();
    if (millis() - bt4time > 500 && bt4Hold) {mainMenu(); waitIDLE = millis(); fromMenu = true;}
    if (screenOff) u8g2.setPowerSave(0);
    screenWait = false;
    screenOff = false;
  } else {
    if (bt4Hold && !fromMenu) {
      if (millis() - waitIDLE < 5000) waitIDLE = millis() - 5000;
      else {
        waitIDLE = millis();
        u8g2.setPowerSave(0);
        screenWait = false;
        screenOff = false;
      }
    }
    fromMenu = false;
    bt4Hold = false;
  }

  // Effects
  if (underGlow) {
    switch (glowType) {
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

  if (rgb && doRainbow) {
    if (millis() - lastRGBUpdate > rgbInterval * 10) {
      lastRGBUpdate = millis();
      static uint16_t hue = 0;
      hue += 128 + rainbowStep;
      l.rainbow(hue, 1, 255, rgbBri, true);
      l.show();
    }
  }

  // Screen
  if (millis() - waitIDLE < screenSaveDuration) {
    switch (usbMode) {
      case 0: keypadMUI(); break;
      case 1: gamepadMUI(); break;
      case 2: mouseMUI(); break;
      default: keypadMUI(); break;
    }
  }
  else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
    if (!screenWait) {
      screenSaver("NoID");
      screenWait = true;
    }
  }
  else {
    if (!screenOff) {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      u8g2.setPowerSave(1);
      screenOff = true;
    }
  }
}
*/

// dear developers, i'm making a big move
// since this project is getting bigger and bigger, i'm trying to split the code so it's more manageable and easy to modify
// though this will take a lot of time, so stay tuned
// - NoID signed -