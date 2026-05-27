#include "global.h"
#include "input.h"
#include "effect.h"
#include "profile.h"
#include "menu.h"
#include "bledev.h"
#include "sandbox.h"

int min(int a, int b) {
  return (a < b) ? a : b;
}

int max(int a, int b) {
  return (a > b) ? a : b;
}

void forceReset() {
  esp_task_wdt_init(1, true);
  esp_task_wdt_add(NULL);
  while (true) {}
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
      u8g2.setFont(u8g2_font_fub20_tf);
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

void importantDebug() {// use in case i messed up the wiring and need to quickly check something without going through the menu
  while (true) {
    updateInput();
    u8g2.clearBuffer();
    // draw 4 buttons
    for (int i = 0; i < 4; i++) {
      if (digitalRead(btnPins[i]) == LOW) {
        u8g2.drawStr(0, 10 + i * 10, ("Btn " + String(i) + " pressed").c_str());
      } else {
        u8g2.drawStr(0, 10 + i * 10, ("Btn " + String(i) + " released").c_str());
      }
    }
    u8g2.sendBuffer();
    delay(100);
  }
} 

void tset() {
  setupSandbox1();
  while (true) {
    loopSandbox1();
  }
}

void mainMenu() {
  const char menu_items[] =
    "Calibration\n"
    "Input handling\n"
    "Filter\n"
    "Effects\n"
    "Connection\n"
    "Other\n"
    "OTA Update\n"
    "About\n"
    "Experimental";

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
  u8g2.setFont(u8g2_font_fub20_tf);
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
      case 4: effectMenu(); break;
      case 5: connectMenu(); break;
      case 6: otherMenu(); break;
      case 7:
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
      case 8: about(); break;
      case 9: tset(); break;
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
    return;
  }
  //importantDebug(); // yes i messed up :'(
  if (!sysLoad()) {
    firstTimeSetup(); // definitely first time
  }
  if (!loadConfig(configPath.c_str())) {
    saveConfig(configPath.c_str()); // fallback to default
  }
  dev.begin();
  waitIDLE = millis();
    if (rgb) {
    l.setBrightness(rgbBri);
    l.fill(l.Color(color[0], color[1], color[2]));
    l.show();
  } else {
    l.setBrightness(0);
    l.show();
  }
}

int rate = 0;
int lastRate = 0;
unsigned long lastRateCheckUpdate = 0;
unsigned long lastIo0Hold = 0;
bool io0Hold = false;

void loop() {

  if (micros() - lastLoopTime < LOOP_INTERVAL_US) return;
  lastLoopTime += LOOP_INTERVAL_US;

  // F4 Key (Menu)
  if (digitalRead(btnPins[3]) == LOW) {
    if (!bt4Hold) {
      bt4Hold = true;
      bt4time = millis();
    }
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

  // Input Handling
  switch (inputHandler) {
    case 0: inputTypeDigitalEmulation(); break;
    case 1: inputTypeHysteresisHandling(); break;
    case 2: inputTypeDynamicActuation(); break;
    default: inputTypeDigitalEmulation(); break;
  }
  if (alwaysReport) needReport = true;

  // USB Report
  if (tud_ready() || alwaysReport) {
    if (needReport) {
      uint8_t keycodes[6] = {0};
      uint8_t idx = 0;
      for (int i = 0; i < 6; i++)
        if (nowPress[i]) keycodes[idx++] = layout[i];
      tud_hid_keyboard_report(dev.report_id, 0, keycodes);
    }
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
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_mf);
    IPAddress ip = WiFi.localIP();
    u8g2.drawStr(0, 10, ("CPU Temp: " + String((int)temperatureRead()) + String((char)0xB0) + "C").c_str());
    if (WiFi.status() != WL_CONNECTED)
      u8g2.drawStr(0, 18, "WiFi: [Off]");
    else u8g2.drawStr(0, 18, ("Device ip: " + ip.toString()).c_str());
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
  rate++;
  if (millis() - lastRateCheckUpdate > 1000) {
    lastRateCheckUpdate = millis();
    lastRate = rate;
    rate = 0;
  }
  // reboot to bootloader if IO0 held while reset
  if (!digitalRead(0) && !io0Hold) {
    io0Hold = true;
    lastIo0Hold = millis();
  } else if (digitalRead(0) && io0Hold) {
    io0Hold = false;
    if (millis() - lastIo0Hold > 2000) {
      forceReset();
    }
  }
}

// dear developers, i'm making a big move
// since this project is getting bigger and bigger, i'm trying to split the code so it's more manageable and easy to modify
// though this will take a lot of time, so stay tuned
// - NoID signed -