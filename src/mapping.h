#pragma once
#include "global.h"

enum InputSource : uint8_t {
  SRC_HALL_0, SRC_HALL_1, SRC_HALL_2,       // analog 0.0–1.0
  SRC_BTN_0,  SRC_BTN_1,  SRC_BTN_2,        // digital
  SRC_GYRO_X, SRC_GYRO_Y, SRC_GYRO_Z,       // °/s
  SRC_ACCEL_X,SRC_ACCEL_Y,SRC_ACCEL_Z,      // m/s²
  SRC_ANGLE_X,SRC_ANGLE_Y,SRC_ANGLE_Z,      // °
  SRC_COUNT
};

enum OutputTarget : uint8_t {
  // Keyboard
  OUT_KEY,                  // gửi HID keycode

  // Gamepad
  OUT_AXIS_LX, OUT_AXIS_LY,
  OUT_AXIS_RX, OUT_AXIS_RY,
  OUT_AXIS_LT, OUT_AXIS_RT,
  OUT_BTN_GP,               // gamepad button index

  // Mouse
  OUT_MOUSE_X, OUT_MOUSE_Y,
  OUT_MOUSE_BTN,
  OUT_MOUSE_WHEEL,
};

struct AxisMap {
  float inMin, inMax;     // ngưỡng input (vd: -70.0, 70.0)
  float outMin, outMax;   // ngưỡng output (vd: -32767, 32767)
  bool  clamp;            // có giới hạn ngoài ngưỡng không
};

struct ThresholdMap {
  float posThresh;   // vd: +30° → trigger
  float negThresh;   // vd: -30° → trigger
  float absThresh;   // vd: |angle| > 45° → trigger
};

union MappingData {
  AxisMap      axis;
  ThresholdMap threshold;
  uint8_t      keycode;    // cho OUT_KEY / OUT_BTN_GP / OUT_MOUSE_BTN
};

enum CombineMode : uint8_t {
  COMBINE_NONE,   // bình thường, 1 source
  COMBINE_POS,    // source này đóng góp phần dương
  COMBINE_NEG,    // source này đóng góp phần âm
};

struct Mapping {
  InputSource  src;
  OutputTarget dst;
  bool         isAxis;     // true: dùng AxisMap, false: ThresholdMap/keycode
  CombineMode  combine;
  MappingData  data;
  uint8_t      keycode;
};

#define MAX_MAPPINGS 24  // đủ cho 15 source × vài target

struct Profile {
  char     name[16];
  uint8_t  usbMode;        // KEYBOARD / GAMEPAD / MOUSE / BLE
  uint8_t  inputHandler;
  float    actuation;
  float    windowSize;
  float    upperThreshold;
  float    lowerThreshold;

  uint8_t  mappingCount;
  Mapping  mappings[MAX_MAPPINGS];
};

struct OutputState {
  uint8_t  keys[6];         // keyboard
  int16_t  axes[6];         // gamepad axes
  uint32_t gpButtons;       // gamepad buttons bitmask
  uint8_t  mouseButtons;
  int8_t   mouseX, mouseY, mouseWheel;
};

float mapf(float x, float inMin, float inMax, float outMin, float outMax, bool clamp);

float readSource(InputSource src);

void writeTarget(OutputState& state, OutputTarget dst, float val, uint8_t extra, CombineMode combine = COMBINE_NONE);

void applyMappings(Profile& p, OutputState& state);