#include "mapping.h"

const char* srcLabels[15] = {
  "Hall0","Hall1","Hall2",
  "Btn0","Btn1","Btn2",
  "GyrX","GyrY","GyrZ",
  "AccX","AccY","AccZ",
  "AngX","AngY","AngZ"
};

const char* dstLabels[12] = {
  "Key",
  "LX","LY","RX","RY","LT","RT",
  "GPBtn",
  "MsX","MsY","MsBtn","MsW"
};

const char* combineLabels[3] = { "", "+", "-" };

bool addAxisMapping(Profile& p, InputSource src, OutputTarget dst,
                    float inMin, float inMax, float outMin, float outMax,
                    bool clamp, CombineMode combine) {
  if (p.mappingCount >= MAX_MAPPINGS) return false;
  Mapping& m = p.mappings[p.mappingCount++];
  m.src     = src;
  m.dst     = dst;
  m.isAxis  = true;
  m.combine = combine;
  m.data.axis = { inMin, inMax, outMin, outMax, clamp };
  return true;
}

bool addThresholdMapping(Profile& p, InputSource src, OutputTarget dst,
                         float posThresh, float negThresh, float absThresh,
                         uint8_t keycode, CombineMode combine) {
  if (p.mappingCount >= MAX_MAPPINGS) return false;
  Mapping& m = p.mappings[p.mappingCount++];
  m.src     = src;
  m.dst     = dst;
  m.isAxis  = false;
  m.combine = combine;
  m.keycode = keycode;
  m.data.threshold = { posThresh, negThresh, absThresh };
  return true;
}

bool editAxisMapping(Profile& p, int index, InputSource src, OutputTarget dst,
                     float inMin, float inMax, float outMin, float outMax,
                     bool clamp, CombineMode combine) {
  if (index < 0 || index >= p.mappingCount) return false;
  Mapping& m = p.mappings[index];
  m.src     = src;
  m.dst     = dst;
  m.isAxis  = true;
  m.combine = combine;
  m.data.axis = { inMin, inMax, outMin, outMax, clamp };
  return true;
}

bool editThresholdMapping(Profile& p, int index, InputSource src, OutputTarget dst,
                          float posThresh, float negThresh, float absThresh,
                          uint8_t keycode, CombineMode combine) {
  if (index < 0 || index >= p.mappingCount) return false;
  Mapping& m = p.mappings[index];
  m.src     = src;
  m.dst     = dst;
  m.isAxis  = false;
  m.combine = combine;
  m.keycode = keycode;
  m.data.threshold = { posThresh, negThresh, absThresh };
  return true;
}

bool removeMapping(Profile& p, int index) {
  if (index < 0 || index >= p.mappingCount) return false;
  for (int i = index; i < p.mappingCount - 1; i++)
    p.mappings[i] = p.mappings[i + 1];
  p.mappingCount--;
  return true;
}

const Mapping* getMapping(const Profile& p, int index) {
  if (index < 0 || index >= p.mappingCount) return nullptr;
  return &p.mappings[index];
}

String mappingToString(const Mapping& m) {
  String s = String(srcLabels[m.src]) + "->" + String(dstLabels[m.dst]);
  if (m.combine != COMBINE_NONE)
    s += String(combineLabels[m.combine]);
  if (m.isAxis)
    s += " [" + String(m.data.axis.inMin,0) + "~" + String(m.data.axis.inMax,0) + "]";
  else
    s += " >" + String(m.data.threshold.posThresh,0);
  return s;
}

float mapf(float x, float inMin, float inMax, float outMin, float outMax, bool clamp) {
  if (inMax - inMin == 0) return outMin; // tránh chia 0
  float mapped = (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
  if (clamp) {
    if (outMin < outMax) {
      mapped = max(outMin, min(mapped, outMax));
    } else {
      mapped = max(outMax, min(mapped, outMin));
    }
  }
  return mapped;
}

float readSource(InputSource src) {
  switch (src) {
    case SRC_HALL_0: return hallVal[0];
    case SRC_HALL_1: return hallVal[1];
    case SRC_HALL_2: return hallVal[2];
    case SRC_BTN_0:  return digitalRead(btnPins[0]) == LOW ? 1.0f : 0.0f;
    case SRC_BTN_1:  return digitalRead(btnPins[1]) == LOW ? 1.0f : 0.0f;
    case SRC_BTN_2:  return digitalRead(btnPins[2]) == LOW ? 1.0f : 0.0f;
    case SRC_GYRO_X: return mpu.getGyroX();
    case SRC_GYRO_Y: return mpu.getGyroY();
    case SRC_GYRO_Z: return mpu.getGyroZ();
    case SRC_ACCEL_X: return mpu.getAccX();
    case SRC_ACCEL_Y: return mpu.getAccY();
    case SRC_ACCEL_Z: return mpu.getAccZ();
    case SRC_ANGLE_X: return mpu.getAngleX();
    case SRC_ANGLE_Y: return mpu.getAngleY();
    case SRC_ANGLE_Z: return mpu.getAngleZ();
    default: return 0.0f;
  }
}

void writeTarget(OutputState& state, OutputTarget dst, float val, uint8_t extra, CombineMode combine) {
  switch (dst) {
    case OUT_KEY:
    if (val > 0.5f) {
        for (int i = 0; i < 6; i++) {
        if (state.keys[i] == extra) break;        // đã có rồi, bỏ qua
        if (state.keys[i] == 0) { state.keys[i] = extra; break; } // thêm vào slot trống
        }
    } else {
        for (int i = 0; i < 6; i++) {
        if (state.keys[i] == extra) { state.keys[i] = 0; break; } // xóa
        }
    }
    break;
    case OUT_AXIS_LX: {
        if (combine == COMBINE_POS)
          state.axes[0] += (int16_t)max(0.0f, val);  // chỉ lấy phần dương
        else if (combine == COMBINE_NEG)
          state.axes[0] += (int16_t)min(0.0f, val);  // chỉ lấy phần âm
        else
          state.axes[0] = (int16_t)val;              // ghi đè bình thường
    } break;
    case OUT_AXIS_LY: {
        if (combine == COMBINE_POS)
          state.axes[1] += (int16_t)max(0.0f, val);
        else if (combine == COMBINE_NEG)
          state.axes[1] += (int16_t)min(0.0f, val);
        else
          state.axes[1] = (int16_t)val;
    } break;
    case OUT_AXIS_RX: {
        if (combine == COMBINE_POS)
          state.axes[2] += (int16_t)max(0.0f, val);
        else if (combine == COMBINE_NEG)
          state.axes[2] += (int16_t)min(0.0f, val);
        else
          state.axes[2] = (int16_t)val;
    } break;
    case OUT_AXIS_RY: {
        if (combine == COMBINE_POS)
          state.axes[3] += (int16_t)max(0.0f, val);
        else if (combine == COMBINE_NEG)
          state.axes[3] += (int16_t)min(0.0f, val);
        else
          state.axes[3] = (int16_t)val;
    } break;
    case OUT_AXIS_LT: {
        if (combine == COMBINE_POS)
          state.axes[4] += (int16_t)max(0.0f, val);
        else if (combine == COMBINE_NEG)
          state.axes[4] += (int16_t)min(0.0f, val);
        else
          state.axes[4] = (int16_t)val;
    } break;
    case OUT_AXIS_RT: {
        if (combine == COMBINE_POS)
          state.axes[5] += (int16_t)max(0.0f, val);
        else if (combine == COMBINE_NEG)
          state.axes[5] += (int16_t)min(0.0f, val);
        else
          state.axes[5] = (int16_t)val;
    } break;
    case OUT_BTN_GP:
      if (val > 0.5f) state.gpButtons |= (1UL << extra);
      else state.gpButtons &= ~(1UL << extra);
      break;
    case OUT_MOUSE_X: state.mouseX = (int8_t)val; break;
    case OUT_MOUSE_Y: state.mouseY = (int8_t)val; break;
    case OUT_MOUSE_WHEEL: state.mouseWheel = (int8_t)val; break;
    case OUT_MOUSE_BTN:
      if (val > 0.5f) state.mouseButtons |= (1 << extra);
      else state.mouseButtons &= ~(1 << extra);
      break;
  }
}

void applyMappings(Profile& p, OutputState& state) {
  for (int i = 0; i < p.mappingCount; i++) {
    Mapping& m = p.mappings[i];
    float val = readSource(m.src);

    if (m.isAxis) {
      float mapped = mapf(val,
        m.data.axis.inMin,  m.data.axis.inMax,
        m.data.axis.outMin, m.data.axis.outMax,
        m.data.axis.clamp);
      writeTarget(state, m.dst, mapped, 0, m.combine);
    } else {
      // threshold → digital
      bool trigger =
        (val >  m.data.threshold.posThresh) ||
        (val < -m.data.threshold.negThresh) ||
        (fabs(val) > m.data.threshold.absThresh);
      writeTarget(state, m.dst, trigger ? 1.0f : 0.0f, m.keycode, m.combine);
    }
  }
}

