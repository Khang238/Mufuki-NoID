#pragma once
#include "global.h"
#include "input.h"

/*
void mgp();

extern NimBLECharacteristic *inputReport;
extern bool deviceConnected;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo&) override {
    deviceConnected = true;
  }
  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo&, int) override {
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};

void marm();
void mwk();
*/

int randRange(int min, int max);

uint8_t charToKey(char c, bool &shift);
