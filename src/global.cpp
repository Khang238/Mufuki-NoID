#include "global.h"

const String ver = "v2.0.0";

int  usbMode = 0;
bool withBLE = false;
int profileVersion = 1; // use newer profile

// Hardware objects
Adafruit_NeoPixel l = Adafruit_NeoPixel(1, 48, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel b = Adafruit_NeoPixel(3, 11, NEO_GRB + NEO_KHZ800);

HIDkeyboard dev;
HIDgamepad gdev;
HIDmouse mdev;
MPU6050 mpu(Wire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// Analog
int deadZone = 32;
float calMax[3] = {2600, 2700, 2800};
float calMin[3] = {2200, 2300, 2300};
float  hallVal[3] = { 0.00,  0.00,  0.00};
int rawVal[3] = {0, 0, 0};
bool doFilter = false;
int filterType = 0;

// Buttons
bool  nowPress[6] = {false};
bool lastPress[6] = {false};
uint8_t layout[6] = {HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_ESCAPE, HID_KEY_F1, HID_KEY_F2};

int inputHandler = 0; // 0: Digital emulation, 1: Hysteresis handling, 2: Dynamic actuation
float actuation = 0.3; // For digital emulation
float upperThreshold = 0.6; // For hysteresis handling
float lowerThreshold = 0.4; // For hysteresis handling
float windowSize = 0.3; // For dynamic actuation
float windowFoot[3] = {0.00, 0.00, 0.00}; // For dynamic actuation

unsigned long pressTime[4] = {0};
bool holding[4] = {false};
bool bt4Hold = false;
unsigned long bt4time = 0;
bool needReport = false;

// Screen
unsigned long waitIDLE = 0;
unsigned long screenSaveDuration = 5000;
unsigned long screenOffDuration  = 5000;
bool screenWait = false;
bool screenOff  = false;
uint8_t screenBri = 1;
int logoType = 0;
String screenLogo = "Mufuki";

int maxBri = 16;
uint32_t lastDecTime = 0;
uint8_t mode = 0;
uint8_t ledOutput[3] = { 0,0,0 };

bool underGlow = false;
int glowType = 0; // 0: Tap, 1: Ripple, 2: Smooth, 3: Burn-in, 4: Soild
bool applyEffect[3] = {false, false, false};
bool rgb = false;
uint8_t rgbBri = 255;
uint8_t color[] = {255, 255, 255};
uint8_t rainbowStep = 1;
bool doRainbow = false;
int updateInterval = 32;
uint8_t rgbInterval = 10; // in n x 10ms
unsigned long lastRGBUpdate = 0;
unsigned long lastUpdate = 0;

// Conectivity & Layouts
bool alwaysReport = false;
int layoutType = 0;
uint32_t LOOP_INTERVAL_US = 1000; // 1ms = 1000Hz
uint32_t lastLoopTime = 0;
bool fromMenu = false;
String btName = "Mufuki";

int rate = 0;
int lastRate = 0;
unsigned long lastRateCheckUpdate = 0;

void screenSaver(const char* title) {
  u8g2.clearBuffer();
  if (logoType > 1 && logoType < 12) {
    u8g2.drawXBMP(20, 20, 88, 232, logoKao[logoType - 2]);
    u8g2.setDrawColor(0);
    u8g2.drawBox(20, 0, 88, 20);
    u8g2.drawBox(20, 40, 88, 24);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 54, title);
  }
  else {
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    u8g2.drawStr((128 - u8g2.getStrWidth(screenLogo.c_str()))/2, 40, screenLogo.c_str());
    u8g2.setFont(u8g2_font_gulim11_t_korean1);
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 54, title);
  }
  u8g2.sendBuffer();
}

void forceReset() {
  esp_task_wdt_init(1, true);
  esp_task_wdt_add(NULL);
  while (true) {}
}