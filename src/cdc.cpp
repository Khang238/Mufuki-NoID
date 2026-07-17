#include "cdc.h"
#include "profile.h"
#include "visplayer.h"
#include "mbedtls/base64.h"

CDCMode cdcMode = CDC_RR;

static File     uploadFile;
static bool     uploading    = false;
static int      lastChunkIdx = -1;

static void sendOk() {
  CDCUSBSerial.println("{\"ok\":true}");
  CDCUSBSerial.flush();
}

static void sendErr(const char* msg) {
  CDCUSBSerial.printf("{\"ok\":false,\"err\":\"%s\"}\n", msg);
  CDCUSBSerial.flush();
}

static void sendSensorEvent() {
  CDCUSBSerial.printf(
    "{\"evt\":\"sensor\",\"data\":{\"h\":[%.3f,%.3f,%.3f],\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}}\n",
    hallVal[0], hallVal[1], hallVal[2],
    mpu.getAngleX(), mpu.getAngleY(), mpu.getAngleZ(),
    mpu.getGyroX(),  mpu.getGyroY(),  mpu.getGyroZ()
  );
  CDCUSBSerial.flush();
}

static void processCommand(const String& raw) {
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, raw) != DeserializationError::Ok) {
    sendErr("parse error"); return;
  }
  const char* cmd = doc["cmd"] | "";

  // ---- ping ----
  if (strcmp(cmd, "ping") == 0) {
    sendOk();

  // ---- mode ----
  } else if (strcmp(cmd, "setMode") == 0) {
    const char* mode = doc["data"] | "rr";
    cdcMode = strcmp(mode, "stream") == 0 ? CDC_STREAM : CDC_RR;
    sendOk();

  // ---- list profiles ----
  } else if (strcmp(cmd, "listProfiles") == 0) {
    std::vector<String> list = listProfiles();
    DynamicJsonDocument resp(512);
    resp["ok"] = true;
    JsonArray arr = resp.createNestedArray("data");
    for (auto& s : list) arr.add(s);
    String out; serializeJson(resp, out);
    CDCUSBSerial.println(out);
    CDCUSBSerial.flush();

  // ---- get profile ----
  } else if (strcmp(cmd, "getProfile") == 0) {
    // serialize prf thành JSON
    DynamicJsonDocument resp(4096);
    resp["ok"] = true;
    JsonObject data = resp.createNestedObject("data");

    data["um"] = usbMode;
    data["bl"] = prf.ble;
    data["ih"] = prf.inputHandler;
    data["at"] = prf.actuation;
    data["ws"] = prf.windowSize;
    data["ut"] = prf.upperThreshold;
    data["lt"] = prf.lowerThreshold;
    data["df"] = prf.doFilter;
    data["ft"] = prf.filterType;
    data["sb"] = prf.screenBri;
    data["ss"] = prf.screenSaveDuration;
    data["so"] = prf.screenOffDuration;
    data["lg"] = prf.logoType;
    data["sl"] = prf.screenLogo;
    data["ug"] = prf.backlight;
    data["gt"] = prf.glowType;
    data["rl"] = prf.rgb;
    data["rb"] = prf.rgbBri;
    data["dr"] = prf.doRainbow;
    data["rs"] = prf.rainbowStep;
    data["ri"] = prf.rgbInterval;
    data["bn"] = prf.btName;
    for (int i = 0; i < 3; i++) data["dz"][i] = prf.deadZone[i];
    for (int i = 0; i < 3; i++) data["cx"][i] = prf.calMax[i];
    for (int i = 0; i < 3; i++) data["cm"][i] = prf.calMin[i];
    for (int i = 0; i < 6; i++) data["lo"][i] = prf.layout[i];
    for (int i = 0; i < 3; i++) data["rc"][i] = prf.color[i];

    JsonArray maps = data.createNestedArray("mp");
    for (int i = 0; i < prf.mappingCount; i++) {
      const Mapping& m = prf.mappings[i];
      JsonObject obj = maps.createNestedObject();
      obj["s"]  = (uint8_t)m.src;
      obj["d"]  = (uint8_t)m.dst;
      obj["ax"] = m.isAxis;
      obj["cb"] = (uint8_t)m.combine;
      obj["kc"] = m.keycode;
      obj["a"]  = m.isAxis ? m.data.axis.inMin       : m.data.threshold.posThresh;
      obj["b"]  = m.isAxis ? m.data.axis.inMax        : m.data.threshold.negThresh;
      obj["c"]  = m.isAxis ? m.data.axis.outMin       : m.data.threshold.absThresh;
      obj["d2"] = m.isAxis ? m.data.axis.outMax       : 0.0f;
      obj["cl"] = m.isAxis ? m.data.axis.clamp        : false;
    }

    String out; serializeJson(resp, out);
    CDCUSBSerial.println(out);
    CDCUSBSerial.flush();

  // ---- set profile (apply vào prf, không lưu file) ----
  } else if (strcmp(cmd, "setProfile") == 0) {
    JsonObject data = doc["data"].as<JsonObject>();
    if (data.isNull()) { sendErr("no data"); return; }

    prf.usbMode        = data["um"] | prf.usbMode;
    prf.ble            = data["bl"] | prf.ble;
    prf.inputHandler   = data["ih"] | prf.inputHandler;
    prf.actuation      = data["at"] | prf.actuation;
    prf.windowSize     = data["ws"] | prf.windowSize;
    prf.upperThreshold = data["ut"] | prf.upperThreshold;
    prf.lowerThreshold = data["lt"] | prf.lowerThreshold;
    prf.doFilter       = data["df"] | prf.doFilter;
    prf.filterType     = data["ft"] | prf.filterType;
    prf.screenBri      = data["sb"] | prf.screenBri;
    prf.screenSaveDuration = data["ss"] | prf.screenSaveDuration;
    prf.screenOffDuration  = data["so"] | prf.screenOffDuration;
    prf.logoType       = data["lg"] | prf.logoType;
    prf.backlight      = data["ug"] | prf.backlight;
    prf.glowType       = data["gt"] | prf.glowType;
    prf.rgb            = data["rl"] | prf.rgb;
    prf.rgbBri         = data["rb"] | prf.rgbBri;
    prf.doRainbow      = data["dr"] | prf.doRainbow;
    prf.rainbowStep    = data["rs"] | prf.rainbowStep;
    prf.rgbInterval    = data["ri"] | prf.rgbInterval;
    if (data["sl"].is<const char*>())
      strncpy(prf.screenLogo, data["sl"].as<const char*>(), sizeof(prf.screenLogo) - 1);
    if (data["bn"].is<const char*>())
      strncpy(prf.btName, data["bn"].as<const char*>(), sizeof(prf.btName) - 1);
    if (data["dz"].is<JsonArray>())
      for (int i = 0; i < 3; i++) prf.deadZone[i] = data["dz"][i] | prf.deadZone[i];
    if (data["cx"].is<JsonArray>())
      for (int i = 0; i < 3; i++) prf.calMax[i] = data["cx"][i] | prf.calMax[i];
    if (data["cm"].is<JsonArray>())
      for (int i = 0; i < 3; i++) prf.calMin[i] = data["cm"][i] | prf.calMin[i];
    if (data["lo"].is<JsonArray>())
      for (int i = 0; i < 6; i++) prf.layout[i] = data["lo"][i] | prf.layout[i];
    if (data["rc"].is<JsonArray>())
      for (int i = 0; i < 3; i++) prf.color[i]  = data["rc"][i] | prf.color[i];

    if (data["mp"].is<JsonArray>()) {
      prf.mappingCount = 0;
      for (JsonObject obj : data["mp"].as<JsonArray>()) {
        if (prf.mappingCount >= MAX_MAPPINGS) break;
        Mapping& m = prf.mappings[prf.mappingCount++];
        m.src     = (InputSource) (obj["s"]  | 0);
        m.dst     = (OutputTarget)(obj["d"]  | 0);
        m.isAxis  =                obj["ax"] | false;
        m.combine = (CombineMode)  (obj["cb"]| 0);
        m.keycode =                obj["kc"] | 0;
        float a = obj["a"] | 0.0f, b = obj["b"] | 0.0f,
              c = obj["c"] | 0.0f, d = obj["d2"]| 0.0f;
        bool cl = obj["cl"] | false;
        if (m.isAxis) m.data.axis      = {a, b, c, d, cl};
        else          m.data.threshold = {a, b, c};
      }
    }
    unpackProfile(prf);
    sendOk();

  // ---- save profile ra file ----
  } else if (strcmp(cmd, "saveProfile") == 0) {
    const char* path = doc["data"] | "/default.json";
    saveProfile(path, prf) ? sendOk() : sendErr("save failed");

  // ---- load profile từ file vào prf ----
  } else if (strcmp(cmd, "loadProfile") == 0) {
    const char* path = doc["data"] | "/default.json";
    loadProfile(path, prf) ? sendOk() : sendErr("load failed");

  // ---- delete profile ----
  } else if (strcmp(cmd, "deleteProfile") == 0) {
    const char* path = doc["data"] | "";
    if (strlen(path) == 0) { sendErr("no path"); return; }
    LittleFS.remove(path) ? sendOk() : sendErr("delete failed");

  // ---- reboot ----
  } else if (strcmp(cmd, "reboot") == 0) {
    sendOk();
    delay(100);
    ESP.restart();

  } else if (strcmp(cmd, "animStart") == 0) {
    const char* name = doc["data"]["name"] | "anim";
    String path = "/" + String(name) + ".vis";
    if (uploading && uploadFile) uploadFile.close();
    uploadFile = LittleFS.open(path, "w");
    if (!uploadFile) { sendErr("open failed"); return; }
    uploading    = true;
    lastChunkIdx = -1;
    sendOk();

  } else if (strcmp(cmd, "animChunk") == 0) {
    if (!uploading) { sendErr("not uploading"); return; }
    int idx = doc["data"]["i"] | -1;
    const char* b64 = doc["data"]["d"] | "";
    if (idx != lastChunkIdx + 1) {
      sendErr("wrong chunk index"); return;
    }
    // decode base64
    size_t b64Len = strlen(b64);
    size_t outLen = 0;
    uint8_t buf[256];
    int ret = mbedtls_base64_decode(buf, sizeof(buf), &outLen,
                                    (const uint8_t*)b64, b64Len);
    if (ret != 0) { sendErr("b64 decode failed"); return; }
    uploadFile.write(buf, outLen);
    uploadFile.flush();
    lastChunkIdx = idx;
    CDCUSBSerial.printf("{\"ok\":true,\"i\":%d}\n", idx);
    CDCUSBSerial.flush();

  } else if (strcmp(cmd, "animEnd") == 0) {
    if (!uploading) { sendErr("not uploading"); return; }
    uploadFile.close();
    uploading = false;
    sendOk();
  } else if (strcmp(cmd, "animList") == 0) {
    // list file .vis
    DynamicJsonDocument resp(512);
    resp["ok"] = true;
    JsonArray arr = resp.createNestedArray("data");
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    while (f) {
      String name = f.name();
      if (name.endsWith(".vis")) arr.add(name);
      f = root.openNextFile();
    }
    String out; serializeJson(resp, out);
    CDCUSBSerial.println(out); CDCUSBSerial.flush();

  } else if (strcmp(cmd, "animPlay") == 0) {
    const char* name = doc["data"] | "";
    String path = "/" + String(name);
    visLoad(path.c_str()) ? sendOk() : sendErr("load failed");

  } else if (strcmp(cmd, "animStop") == 0) {
    visStop(); sendOk();
  } else if (strcmp(cmd, "rmani") == 0) {
    LittleFS.remove("/idle.vis");
  }
   else {
    sendErr("unknown command");
  }
}

void handleCDC() {
  if (!CDCUSBSerial) return;
  if (cdcMode == CDC_STREAM) sendSensorEvent();
  if (CDCUSBSerial.available()) {
    String raw = CDCUSBSerial.readStringUntil('\n');
    raw.trim();
    if (raw.length() > 0) processCommand(raw);
  }
}