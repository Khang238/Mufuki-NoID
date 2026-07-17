#include "global.h"
#include "profile.h"

const String ver = "v2.8.6";
bool firstTime = false;

int  usbMode = 0;
bool withBLE = false;
int profileVersion = 1; // use newer profile
int vpidSet = 0;
volatile bool menuOpen = false;


// Hardware objects
Adafruit_NeoPixel l = Adafruit_NeoPixel(1, 48, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel b = Adafruit_NeoPixel(3, 11, NEO_GRB + NEO_KHZ800);

HIDkeyboard dev;
HIDgamepad gdev;
HIDmouse mdev;
CDCusb CDCUSBSerial;

// BleGamepad* gblue;
// BleKeyboard* kblue;
// BleMouse* mlue;

MPU6050 mpu(Wire);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// Analog
float  hallVal[3] = { 0.00,  0.00,  0.00};
int rawVal[3] = {0, 0, 0};

// Buttons
bool  nowPress[6] = {false};
bool lastPress[6] = {false};
float windowFoot[3] = {0.00, 0.00, 0.00};

unsigned long pressTime[4] = {0};
bool holding[4] = {false};
bool bt4Hold = false;
unsigned long bt4time = 0;
bool needReport = false;

// Screen
unsigned long waitIDLE = 0;
bool screenWait = false;
bool screenOff  = false;
bool timeUpdated = false;
struct tm timeinfo;

int maxBri = 16;
uint32_t lastDecTime = 0;
uint8_t mode = 0;
uint8_t ledOutput[3] = { 0,0,0 };
bool morseKey = false;

// bool backlight = false;
bool applyEffect[3] = {false, false, false};
int updateInterval = 32;
unsigned long lastRGBUpdate = 0;
unsigned long lastUpdate = 0;

// Conectivity & Layouts
bool alwaysReport = false;
// int layoutType = 0;
uint32_t LOOP_INTERVAL_US = 1000; // 1ms = 1000Hz
uint32_t lastLoopTime = 0;
bool fromMenu = false;
String btName = "Mufuki";

int rate = 0;
int lastRate = 0;
unsigned long lastRateCheckUpdate = 0;

void globFont() {
  u8g2.setFont(u8g2_font_gulim_11_idk);
}

void screenSaver(const char* title) {
  u8g2.clearBuffer();
  if (prf.logoType > 1 && prf.logoType < 12) {
    u8g2.drawXBMP(20, 20, 88, 20, logoKao[prf.logoType - 2]);
    u8g2.setDrawColor(0);
    u8g2.drawBox(20, 0, 88, 20);
    u8g2.drawBox(20, 40, 88, 24);
    u8g2.setDrawColor(1);
    globFont();
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 54, title);
  }
  else {
    u8g2.setFont(u8g2_font_spleen16x32_mr);
    u8g2.drawStr((128 - u8g2.getStrWidth(prf.screenLogo))/2, 40, prf.screenLogo);
    globFont();
    u8g2.drawStr((128 - u8g2.getStrWidth(title))/2, 54, title);
  }
  u8g2.sendBuffer();
}

void forceReset() {
  esp_task_wdt_init(1, true);
  esp_task_wdt_add(NULL);
  while (true) {}
}