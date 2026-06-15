#include "bledev.h"

void mgp() {
  applyMappings(prf, mos);
  if (!gblue->isConnected()) return;
  for (uint8_t i = 0; i < 16; i++) {
    if (mos.gpButtons & (1UL << i))
      gblue->press(i + 1);
    else
      gblue->release(i + 1);
  }

  gblue->setAxes(
    mos.axes[0],
    mos.axes[1],
    mos.axes[2],
    mos.axes[4],
    mos.axes[5],
    mos.axes[3]
  );

  gblue->setHat1(HAT_CENTERED);
  gblue->sendReport();
}

void marm() {
  uint8_t m[5];
  m[0] = mos.mouseButtons;
  m[1] = -mos.mouseX;
  m[2] = -mos.mouseY;
  m[3] = mos.mouseWheel;
  m[4] = 0;
  mlue->inputMouse->setValue(m, 5);
  mlue->inputMouse->notify();
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

void mwk() {
  KeyReport report = {0};
  uint8_t idx = 0;
  for (int i = 0; i < 6; i++)
    if (nowPress[i]) report.keys[idx++] = layout[i];
  kblue->sendReport(&report);
}