#pragma once
#include "global.h"

extern const char buttonName[];

extern const char* buttonString[];

extern const uint8_t buttonCode[];

int codeToIndex(uint8_t code);

String codeToName(uint8_t code);

uint8_t nameToCode(String name);