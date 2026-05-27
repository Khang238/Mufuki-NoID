#include "sandbox.h"

Profile testProfile;
OutputState testOutputState;

Profile createGamepadTestProfile() {
  Profile p = {};
  strncpy(p.name, "Gamepad Test", 16);
  p.usbMode      = MODE_GAMEPAD;
  p.inputHandler = 0;
  p.actuation    = 0.3f;
  p.windowSize   = 0.3f;
  p.mappingCount = 0;

  auto add = [&](InputSource src, OutputTarget dst, bool isAxis,
                 float a, float b, float c, float d, bool clamp, uint8_t keycode) {
    Mapping& m = p.mappings[p.mappingCount++];
    m.src    = src;
    m.dst    = dst;
    m.isAxis = isAxis;
    if (isAxis) {
      m.data.axis = { a, b, c, d, clamp };
    } else {
      m.data.threshold = { a, b, c };
      m.keycode   = keycode;
    }
  };

  // IMU → joystick trái
  add(SRC_ANGLE_X, OUT_AXIS_LX, true,  -60.0f, 60.0f, -32767, 32767, true, 0);
  add(SRC_ANGLE_Y, OUT_AXIS_LY, true,  -60.0f, 60.0f, -32767, 32767, true, 0);

  // Hall → trigger trái/phải
  add(SRC_HALL_0,  OUT_AXIS_LT, true,  0.0f, 1.0f, 0, 32767, true, 0);
  add(SRC_HALL_1,  OUT_AXIS_RT, true,  0.0f, 1.0f, 0, 32767, true, 0);

  // Hall 2 + BTN → gamepad buttons
  add(SRC_HALL_2,  OUT_BTN_GP,  false, 0.5f, 0.0f, 0.0f, 0, false, 0); // btn 0
  add(SRC_BTN_0,   OUT_BTN_GP,  false, 0.5f, 0.0f, 0.0f, 0, false, 1); // btn 1
  add(SRC_BTN_1,   OUT_BTN_GP,  false, 0.5f, 0.0f, 0.0f, 0, false, 2); // btn 2

  return p;
}

void setupSandbox1() {
  testProfile = createGamepadTestProfile();
  u8g2.setFont(u8g2_font_5x8_mf);
}

void loopSandbox1() {
  // backend
  updateInput();
  mpu.update();
  applyMappings(testProfile, testOutputState);

  // frontend
  u8g2.clearBuffer();
  // draw 16 squares representing the state of the gamepad buttons
  // then 6 bars representing the state of the axes
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 6; x++) {
      int idx = y * 6 + x;
      if (idx >= 6) break;
      if (testOutputState.gpButtons & (1UL << idx)) {
        u8g2.drawBox(10 + x * 10, 10 + y * 10, 8, 8);
      } else {
        u8g2.drawFrame(10 + x * 10, 10 + y * 10, 8, 8);
      }
    }
  }
  for (int i = 0; i < 6; i++) {
    int len = map(testOutputState.axes[i], -32767, 32767, -20, 20);
    if (len > 0) {
      u8g2.drawBox(10 + i * 10, 30 - len, 8, len);
    } else {
      u8g2.drawBox(10 + i * 10, 30, 8, -len);
    }
  }
  u8g2.sendBuffer();
}

Profile prf = {};
HIDgamepad devgp;

void setupSandbox2() {
  tud_disconnect();
  delay(1000);
  devgp.begin();
  tud_connect();
  testProfile = createGamepadTestProfile();
}

void loopSandbox2() {
  updateInput();
  mpu.update();
  applyMappings(testProfile, testOutputState);

  // gửi state ra gamepad
  devgp.joystick1(testOutputState.axes[0], testOutputState.axes[1], 0);
  devgp.joystick2(testOutputState.axes[2], testOutputState.axes[3], 0);
  devgp.buttons(testOutputState.gpButtons);

  u8g2.clearBuffer();
  // draw 16 squares representing the state of the gamepad buttons
  // then 6 bars representing the state of the axes
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 6; x++) {
      int idx = y * 6 + x;
      if (idx >= 6) break;
      if (testOutputState.gpButtons & (1UL << idx)) {
        u8g2.drawBox(10 + x * 10, 10 + y * 10, 8, 8);
      } else {
        u8g2.drawFrame(10 + x * 10, 10 + y * 10, 8, 8);
      }
    }
  }
  for (int i = 0; i < 6; i++) {
    int len = map(testOutputState.axes[i], -32767, 32767, -20, 20);
    if (len > 0) {
      u8g2.drawBox(10 + i * 10, 30 - len, 8, len);
    } else {
      u8g2.drawBox(10 + i * 10, 30, 8, -len);
    }
  }
  u8g2.sendBuffer();
}