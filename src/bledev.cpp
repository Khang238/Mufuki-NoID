#include "bledev.h"

void mgp() {
  l.setBrightness(255);
  BleGamepad g(btName.c_str(), "NoID", 100);
  BleGamepadConfiguration cfg;
  cfg.setAutoReport(false);
  g.begin(&cfg);
  bool running = true;
  bool enableThumbs = false;
  unsigned long lps = millis();
  waitIDLE = millis();
  screenWait = false;
  screenOff = false;
  while (running) {
    // updateInput();
    mpu.update();
    int normLx = (int)((hallVal[2] - hallVal[1] + 1.0f) * 16383.5f); // [-1..1] → [0..32767]
    int normLy = (int)((1.00f - hallVal[0]) * 32767.0f);                       // [0..1] → [0..32767]
    int gRx    = 32767 - (int)((constrain(-mpu.getAngleY(), -70, 70) + 70) * 234); // [-90..90] → [0..32767]
    int gRy    = 32767 - (int)((constrain(-mpu.getAngleX(), -70, 70) + 70) * 234);
    int button = getButton();
    if (nowPress[3]) g.press(1); else g.release(1); // A
    if (nowPress[4]) g.press(2); else g.release(2); // B
    if (button == 2) {
      if (millis() - pressTime[2] < 1000)
        enableThumbs = (screenOff || screenWait) ? enableThumbs : !enableThumbs;
      else {
        u8g2.clearBuffer();
        u8g2.drawStr(64 - u8g2.getStrWidth("Calibrating") / 2, 32, "Calibrating...");
        u8g2.sendBuffer();
        mpu.calcOffsets();
        delay(200);
      }
      waitIDLE = millis();
      screenOff = false;
      screenWait = false;
      u8g2.setPowerSave(0);
    }
    if (button == 3) running = false; // Exit
    if (!enableThumbs) {normLx = 16383; normLy = 16383; gRx = 16383; gRy = 16383;}
    g.setLeftThumb(normLx, normLy);
    g.setRightThumb(gRx, gRy);
    g.sendReport();
    if (millis() - waitIDLE < screenSaveDuration) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.drawStr(0, 12, "BLE Mode [Gamepad]");
      l.fill(l.Color(0, 255, 255));
      if (!g.isConnected()) {u8g2.drawStr(0, 20, "Not Connected"); l.fill(l.Color(255, 0, 0));}
      else if (!enableThumbs) {u8g2.drawStr(0, 20, "Thumbs Disabled"); l .fill(l.Color(255, 255, 0));}
      else {
        String tmp = "rx:" + String(gRx) + ", ry:" + String(gRy);
        u8g2.drawStr(0, 20, tmp.c_str());
      }
      u8g2.drawFrame(4, 26, 32, 32);
      int cputmp = temperatureRead();
      String tmp = String(cputmp) + String((char)(0xB0)) + "C";
      u8g2.drawStr(64 - u8g2.getStrWidth(tmp.c_str()) / 2, 42, tmp.c_str());
      u8g2.drawFrame(92, 26, 32, 32);
      if (enableThumbs) {
        int cLx = map(normLx, 0, 32767, 6, 34);
        int cLy = map(normLy, 0, 32767, 28, 56);
        u8g2.drawCircle(cLx, cLy, 2, U8G2_DRAW_ALL);
        int cRx = map(gRx, 0, 32767, 96, 122);
        int cRy = map(gRy, 0, 32767, 28, 56);
        u8g2.drawCircle(cRx, cRy, 2, U8G2_DRAW_ALL);
      } else {
        u8g2.drawCircle(18, 42, 2, U8G2_DRAW_ALL);
        u8g2.drawCircle(106, 42, 2, U8G2_DRAW_ALL);
      }
      u8g2.sendBuffer();
      l.show();
    }
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("BLE Gamepad");
        screenWait = true;
        l.fill(l.Color(0, 0, 255));
        l.show();
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
        l.fill(l.Color(255, 0, 255));
        l.show();
      }
    }
    if (millis() - lps < 10) delay(millis() - lps);
    lps = millis();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(64 - u8g2.getStrWidth("Exiting...") / 2, 32, "Exiting...");
  u8g2.sendBuffer();
  g.end();
  BLEDevice::deinit(true);
  NimBLEDevice::deinit(true);
  delay(1000);
  l.setBrightness(0);
  l.show();
  u8g2.setPowerSave(0);
}

NimBLECharacteristic *inputReport;
bool deviceConnected = false;

/*
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo&) override {
    deviceConnected = true;
  }
  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo&, int) override {
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};
*/

// i'm the crazy one
void marm() {
  l.setBrightness(255);
  
  static const uint8_t hrd[] = {
    USAGE_PAGE(1), 0x01,  // USAGE_PAGE (Generic Desktop)
    USAGE(1), 0x02,       // USAGE (Mouse)
    COLLECTION(1), 0x01,  // COLLECTION (Application)
    USAGE(1), 0x01,       //   USAGE (Pointer)
    COLLECTION(1), 0x00,  //   COLLECTION (Physical)
    // ------------------------------------------------- Buttons (Left, Right, Middle, Back, Forward)
    USAGE_PAGE(1), 0x09,       //     USAGE_PAGE (Button)
    USAGE_MINIMUM(1), 0x01,    //     USAGE_MINIMUM (Button 1)
    USAGE_MAXIMUM(1), 0x05,    //     USAGE_MAXIMUM (Button 5)
    LOGICAL_MINIMUM(1), 0x00,  //     LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0x01,  //     LOGICAL_MAXIMUM (1)
    REPORT_SIZE(1), 0x01,      //     REPORT_SIZE (1)
    REPORT_COUNT(1), 0x05,     //     REPORT_COUNT (5)
    HIDINPUT(1), 0x02,         //     INPUT (Data, Variable, Absolute) ;5 button bits
    // ------------------------------------------------- Padding
    REPORT_SIZE(1), 0x03,   //     REPORT_SIZE (3)
    REPORT_COUNT(1), 0x01,  //     REPORT_COUNT (1)
    HIDINPUT(1), 0x03,      //     INPUT (Constant, Variable, Absolute) ;3 bit padding
    // ------------------------------------------------- X/Y position, Wheel
    USAGE_PAGE(1), 0x01,       //     USAGE_PAGE (Generic Desktop)
    USAGE(1), 0x30,            //     USAGE (X)
    USAGE(1), 0x31,            //     USAGE (Y)
    USAGE(1), 0x38,            //     USAGE (Wheel)
    LOGICAL_MINIMUM(1), 0x81,  //     LOGICAL_MINIMUM (-127)
    LOGICAL_MAXIMUM(1), 0x7f,  //     LOGICAL_MAXIMUM (127)
    REPORT_SIZE(1), 0x08,      //     REPORT_SIZE (8)
    REPORT_COUNT(1), 0x03,     //     REPORT_COUNT (3)
    HIDINPUT(1), 0x06,         //     INPUT (Data, Variable, Relative) ;3 bytes (X,Y,Wheel)
    // ------------------------------------------------- Horizontal wheel
    USAGE_PAGE(1), 0x0c,       //     USAGE PAGE (Consumer Devices)
    USAGE(2), 0x38, 0x02,      //     USAGE (AC Pan)
    LOGICAL_MINIMUM(1), 0x81,  //     LOGICAL_MINIMUM (-127)
    LOGICAL_MAXIMUM(1), 0x7f,  //     LOGICAL_MAXIMUM (127)
    REPORT_SIZE(1), 0x08,      //     REPORT_SIZE (8)
    REPORT_COUNT(1), 0x01,     //     REPORT_COUNT (1)
    HIDINPUT(1), 0x06,         //     INPUT (Data, Var, Rel)
    END_COLLECTION(0),         //   END_COLLECTION
    END_COLLECTION(0)          // END_COLLECTION
  };

  NimBLEDevice::init(btName.c_str());

  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService *hidService = pServer->createService("1812"); // HID Service

  NimBLECharacteristic *reportMap =
      hidService->createCharacteristic("2A4B", NIMBLE_PROPERTY::READ);
  reportMap->setValue(hrd, sizeof(hrd));

  inputReport = hidService->createCharacteristic(
      "2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  hidService->start();

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->setAppearance(0x03C2); // HID Mouse
  advertising->addServiceUUID(hidService->getUUID());
  advertising->start();

  bool running = true;
  bool enableMove = false;
  unsigned long lps = millis();
  waitIDLE = millis();
  screenWait = false;
  screenOff = false;

  while (running) {
    // updateInput();
    mpu.update();
    int mvx = constrain(-mpu.getGyroZ() /  8, -10, 10);
    int mvy = constrain(-mpu.getGyroX()  / 8, -10, 10);
    int button = getButton();
    uint8_t brp = 0;
    int scroll = 0;
    if (nowPress[0]) brp = brp | 1;
    if (nowPress[1]) brp = brp | 4;
    if (nowPress[2]) brp = brp | 2;
    if (nowPress[3]) scroll = 1; // Up
    else if (nowPress[4]) scroll = -1; // Down
    uint8_t report[5] = {brp, mvx, mvy, scroll, 0};
    inputReport->setValue(report, sizeof(report));
    inputReport->notify();
    if (enableMove) {
      inputReport->setValue(report, sizeof(report));
      inputReport->notify();
    }
    else if (scroll != 0) {
      inputReport->setValue(report, sizeof(report));
      inputReport->notify();
    }
    if (button == 2) {
      if (millis() - pressTime[2] < 1000)
        enableMove = (screenOff || screenWait) ? enableMove : !enableMove;
      else {
        u8g2.clearBuffer();
        u8g2.drawStr(64 - u8g2.getStrWidth("Calibrating") / 2, 32, "Calibrating...");
        u8g2.sendBuffer();
        mpu.calcOffsets();
        delay(200);
      }
      waitIDLE = millis();
      screenOff = false;
      screenWait = false;
      u8g2.setPowerSave(0);
    }
    if (button == 3) running = false;
    if (millis() - waitIDLE < screenSaveDuration) {
      u8g2.clearBuffer();
      l.fill(l.Color(0, 255, 255));
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      u8g2.drawStr((128 - u8g2.getStrWidth("BLE Mouse"))/2, 24, "BLE Mouse");
      String tmp = "Temp: " + String((int)temperatureRead()) + String((char)(0xB0)) + "C";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 48, tmp.c_str());
      tmp = "gx:" + String((int)mpu.getGyroX()) + ", gz:" + String((int)mpu.getGyroZ());
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 12, tmp.c_str());
      if (!deviceConnected) {
        u8g2.drawStr((128 - u8g2.getStrWidth("Not Connected"))/2, 36, "Not Connected");
        l.fill(l.Color(255, 120, 0));
      } else if (!enableMove) {
        u8g2.drawStr((128 - u8g2.getStrWidth("Gyro Disabled"))/2, 36, "Gyro Disabled");
        l.fill(l.Color(255, 120, 0));
      }
      l.show();
      u8g2.sendBuffer();
    }
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("BLE Mouse");
        screenWait = true;
        l.fill(l.Color(0, 0, 255));
        l.show();
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
        l.fill(l.Color(255, 0, 255));
        l.show();
      }
    }
    if (millis() - lps < 8) delay(millis() - lps);
    lps = millis();
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(64 - u8g2.getStrWidth("Exiting...") / 2, 32, "Exiting...");
  u8g2.sendBuffer();
  NimBLEDevice::deinit(true);
  delay(1000);
  l.setBrightness(0);
  l.show();
}

int randRange(int min, int max) {return min + esp_random() % (max - min + 1);}

uint8_t charToKey(char c, bool &shift) {
  shift = false;
  if (c >= 'a' && c <= 'z') {
    return HID_KEY_A + (c - 'a');   // a..z
  }
  if (c >= 'A' && c <= 'Z') {
    shift = true;
    return HID_KEY_A + (c - 'A');   // A..Z (shift)
  }
  if (c >= '1' && c <= '9') {
    return HID_KEY_1 + (c - '1');   // 1..9
  }
  if (c == '0') {
    return HID_KEY_0;                // 0
  }

  if (c = ' ') return HID_KEY_SPACE;
  return 0; // unsupported
}

// short story: NEVER trust bluetooth libraries
// but i use raw reporting for almost all of them (usb, ble mouse, ble keyboard, gamepad is good though)
// BECAUSE ALL OF THEM SUCKS
// like, bro, just think about it, who tf genius point the BleKeyboard::print() to some mf system print that i have no fking ideal???

void mwk() {
  l.setBrightness(255);
  BleKeyboard kb(btName.c_str(), "NoID", 100);
  kb.begin();
  bool running = true;
  waitIDLE = millis();
  screenWait = false;
  screenOff = false;
  while (running) {
    // updateInput();
    // Report
    if (kb.isConnected()) {
      if (needReport) {
        KeyReport report = {0};
        report.modifiers = 0;
        uint8_t idx = 0;
        for (int i = 0; i < 6; i++)
          if (nowPress[i]) report.keys[idx++] = layout[i];
        kb.sendReport(&report);
      }
    }
    bool button = getButton() == 3;
    if (button) {
      if (millis() - pressTime[3] > 1000) running = false;
      else {
        if (screenOff) u8g2.setPowerSave(0);
        screenWait = false;
        screenOff = false;
        waitIDLE = millis();
      }
    }
    mpu.update();
    if (abs(mpu.getAngleY()) > 80 && button) {
      if (screenOff) u8g2.setPowerSave(0);
      screenWait = false;
      screenOff = false;
      waitIDLE = millis();
      applyEffect[0] = false;
      applyEffect[1] = false;
      applyEffect[2] = false;
      String text = keyboard("");
      int lineCount = 0;
      u8g2.clearBuffer();
      if (text != "") {
        for (int i = 0; i < text.length(); i++) {
          bool shift;
          uint8_t key = charToKey((char)text[i], shift);
          if (key) {
            KeyReport report = {0};
            report.modifiers = shift ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
            report.keys[0] = key;
            kb.sendReport(&report);
            delay(randRange(10, 80));
            report.modifiers = 0;
            report.keys[0] = 0;
            kb.sendReport(&report);
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
      if (screenOff) u8g2.setPowerSave(0);
      screenWait = false;
      screenOff = false;
      waitIDLE = millis();
      mpu.update();
    }
    if (millis() - waitIDLE < screenSaveDuration) {
      u8g2.clearBuffer();
      l.fill(l.Color(0, 255, 255));
      u8g2.setFont(u8g2_font_gulim11_t_korean1);
      u8g2.drawStr((128 - u8g2.getStrWidth("BLE Keyboard"))/2, 24, "BLE Keyboard");
      String tmp = "Temp: " + String((int)temperatureRead()) + String((char)(0xB0)) + "C";
      u8g2.drawStr((128 - u8g2.getStrWidth(tmp.c_str()))/2, 48, tmp.c_str());
      if (!kb.isConnected()) {
        u8g2.drawStr((128 - u8g2.getStrWidth("Not Connected"))/2, 36, "Not Connected");
        l.fill(l.Color(255, 120, 0));
      }
      l.show();
      u8g2.sendBuffer();
    }
    else if (millis() - waitIDLE < screenSaveDuration + screenOffDuration) {
      if (!screenWait) {
        screenSaver("BLE Keyboard");
        screenWait = true;
        l.fill(l.Color(0, 0, 255));
        l.show();
      }
    }
    else {
      if (!screenOff) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        screenOff = true;
        l.fill(l.Color(255, 0, 255));
        l.show();
      }
    }
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_gulim11_t_korean1);
  u8g2.drawStr(64 - u8g2.getStrWidth("Exiting...") / 2, 32, "Exiting...");
  u8g2.sendBuffer();
  kb.end();
  BLEDevice::deinit(true);
  delay(1000);
  l.setBrightness(0);
  l.show();
}