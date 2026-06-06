// cdc.h
#pragma once
#include "global.h"
#include "cdcusb.h"

enum CDCMode : uint8_t { CDC_RR, CDC_STREAM };
extern CDCMode cdcMode;
extern CDCusb CDCUSBSerial;

void handleCDC();