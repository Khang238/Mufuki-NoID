#pragma once
#include "global.h"

enum InputSource : uint8_t {
  SRC_HALL_0, SRC_HALL_1, SRC_HALL_2,
  SRC_BTN_0,  SRC_BTN_1,  SRC_BTN_2,
  SRC_GYRO_X, SRC_GYRO_Y, SRC_GYRO_Z,
  SRC_ACCEL_X,SRC_ACCEL_Y,SRC_ACCEL_Z,
  SRC_ANGLE_X,SRC_ANGLE_Y,SRC_ANGLE_Z,
  SRC_COUNT
};

enum OutputTarget : uint8_t {
  OUT_KEY,

  OUT_AXIS_LX, OUT_AXIS_LY,
  OUT_AXIS_RX, OUT_AXIS_RY,
  OUT_AXIS_LT, OUT_AXIS_RT,
  OUT_BTN_GP,

  OUT_MOUSE_X, OUT_MOUSE_Y,
  OUT_MOUSE_BTN,
  OUT_MOUSE_WHEEL,
};

struct AxisMap {
  float inMin, inMax;
  float outMin, outMax;
  bool  clamp;
};

struct ThresholdMap {
  float posThresh;
  float negThresh;
  float absThresh;
};

union MappingData {
  AxisMap      axis;
  ThresholdMap threshold;
  uint8_t      keycode;
};

enum CombineMode : uint8_t {
  COMBINE_NONE,
  COMBINE_POS,
  COMBINE_NEG,
};

struct Mapping {
  InputSource  src;
  OutputTarget dst;
  bool         isAxis;
  CombineMode  combine;
  MappingData  data;
  uint8_t      keycode;
};

// Labels
extern const char* srcLabels[15];

extern const char* dstLabels[12];

extern const char* combineLabels[3];

#define MAX_MAPPINGS 24

struct Profile {
  uint8_t usbMode = 0;
  bool    ble = false;
  uint32_t LIU = 1000;

  // Input
  uint8_t inputHandler = 0;
  float   actuation = 0.3;
  float   windowSize = 0.3;
  float   upperThreshold = 0.6;
  float   lowerThreshold = 0.4;
  float   calMax[3] = {2200, 2200, 2200};
  float   calMin[3] = {2000, 2000, 2000};
  int     deadZone[3] = {16, 16, 16};
  bool    doFilter = false;
  int     ovsSamples = 16;
  float   emaAlpha = 0.05;
  uint8_t filterType = 0;
  uint8_t layout[6] = {HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2};

  // Display
  uint8_t       screenBri = 128;
  unsigned long screenSaveDuration = 5000;
  unsigned long screenOffDuration = 10000;
  int           logoType = 0;
  char          screenLogo[32] = "Mufuki";

  // Effects
  bool    underGlow = false;
  uint8_t glowType = 0;
  bool    rgb = false;
  uint8_t rgbBri = 255;
  uint8_t color[3] = {255, 255, 255};
  bool    doRainbow = false;
  uint8_t rainbowStep = 0;
  uint8_t rgbInterval = 100;

  // BLE
  char btName[32] = "Mufuki";

  // Mapping
  uint8_t mappingCount;
  Mapping mappings[MAX_MAPPINGS];
};

struct OutputState {
  uint8_t  keys[6];         // keyboard
  int16_t  axes[6];         // gamepad axes
  uint32_t gpButtons;       // gamepad buttons bitmask
  uint8_t  mouseButtons;
  int8_t   mouseX, mouseY, mouseWheel;
};

bool addAxisMapping(Profile& p, InputSource src, OutputTarget dst,
                    float inMin, float inMax, float outMin, float outMax,
                    bool clamp, CombineMode combine = COMBINE_NONE);

bool addThresholdMapping(Profile& p, InputSource src, OutputTarget dst,
                         float posThresh, float negThresh, float absThresh,
                         uint8_t keycode, CombineMode combine = COMBINE_NONE);

bool editAxisMapping(Profile& p, int index, InputSource src, OutputTarget dst,
                     float inMin, float inMax, float outMin, float outMax,
                     bool clamp, CombineMode combine = COMBINE_NONE);

bool editThresholdMapping(Profile& p, int index, InputSource src, OutputTarget dst,
                          float posThresh, float negThresh, float absThresh,
                          uint8_t keycode, CombineMode combine = COMBINE_NONE);

const Mapping* getMapping(const Profile& p, int index);
String mappingToString(const Mapping& m);

bool removeMapping(Profile& p, int index);
                         
float mapf(float x, float inMin, float inMax, float outMin, float outMax, bool clamp);

float readSource(InputSource src);

void writeTarget(OutputState& state, OutputTarget dst, float val, uint8_t extra, CombineMode combine = COMBINE_NONE);

void applyMappings(Profile& p, OutputState& state);