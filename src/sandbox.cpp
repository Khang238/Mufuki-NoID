#include "sandbox.h"

void gitDownload() {
  WiFiClientSecure client;
  client.setInsecure(); 

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 8, "Connecting to GitHub...");
  u8g2.sendBuffer();

  if (http.begin(client, file_url)) {
    int httpCode = http.GET();  
    if (httpCode == HTTP_CODE_OK) {
      u8g2.drawStr(0, 16, "Connected! Downloading...");
      u8g2.sendBuffer();

      File file = LittleFS.open("/animation.vis", FILE_WRITE);
      if (!file) {
        u8g2.drawStr(0, 24, "Failed to open file!");
        u8g2.sendBuffer();
        return;
      }

      WiFiClient* stream = http.getStreamPtr();
      uint8_t buffer[128];
      int len = http.getSize();
      int totalDownloaded = 0;
      bool done = false;

      while ((http.connected() && (len > 0 || len == -1)) && !done) {
        size_t size = stream->available();
        if (size) {
          int c = stream->readBytes(buffer, ((size > sizeof(buffer)) ? sizeof(buffer) : size));
          file.write(buffer, c);
          
          totalDownloaded += c;
          if (len > 0) {
            int progress = (totalDownloaded * 100) / len;
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_5x8_tr);
            u8g2.drawStr(0, 8, "Downloading...");
            u8g2.setCursor(0, 16);
            u8g2.print("Progress: ");
            u8g2.print(progress);
            u8g2.print("%");
            u8g2.sendBuffer();
          }
          done = (len > 0 && totalDownloaded >= len);
        } else {
          delay(1);
        }
        delay(1);
      }
      file.close();
      u8g2.drawStr(0, 32, "Download completed!");
      u8g2.sendBuffer();
      } else {
          u8g2.drawStr(0, 24, "HTTP Error!");
          String tmp = "Code: " + String(httpCode);
          u8g2.drawStr(0, 32, tmp.c_str());
          u8g2.sendBuffer();
      }
      http.end();
  } else {
      u8g2.drawStr(0, 24, "Failed to connect to server!");
      u8g2.sendBuffer();
  }
  u8g2.drawStr(0, 40, "Press any button to continue...");
  u8g2.sendBuffer();
  while (getButton() == -1) { delay(10); }
}

void clockCheck() {
  while (getButton() > -1) { delay(10); }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  WiFi.mode(WIFI_STA);
  u8g2.drawStr(0, 8, "WiFi Enabled, waiting for connection...");
  u8g2.sendBuffer();
  while (!WiFi.isConnected() && getButton() == -1) delay(10);
  u8g2.drawStr(0, 16, "Connected! Syncing time...");
  u8g2.sendBuffer();
  configTime(prf.GMTPlus * 3600, 0, "time.google.com", "time.nist.gov");
  bool success = getLocalTime(&timeinfo);
  if (success) {
    u8g2.drawStr(0, 24, "Time synced successfully!");
    u8g2.drawStr(0, 32, "Current time: ");
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    u8g2.drawStr(0, 40, timeStr);
  } else {
    u8g2.drawStr(0, 24, "Failed to sync time!");
  }
  u8g2.sendBuffer();
  while (getButton() == -1) { delay(10); }
}

void macroTest() {
  while (getButton() > -1) { delay(10); }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  Macro mc;
  u8g2.drawStr(0, 8, "Macro created");
  u8g2.sendBuffer();
  addAct(mc, HID_KEY_Z, MACRO_PRESS);
  addAct(mc, HID_KEY_X, MACRO_PRESS);
  addAct(mc, HID_KEY_C, MACRO_PRESS);
  u8g2.drawStr(0, 16, "Added zxc, executing");
  u8g2.sendBuffer();
  delay(1000);
  executeMacro(mc);
  delay(1000);
  u8g2.drawStr(0, 24, "try adding text and execute");
  u8g2.sendBuffer();
  addTextAct(mc, "Mufuki - An Osu! keypad project", 32);
  executeMacro(mc);
  u8g2.drawStr(0, 32, "done");
  u8g2.sendBuffer();
  while (getButton() > -1) { delay(10); }
}