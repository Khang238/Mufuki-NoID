#pragma once
#include "global.h"
#include "mapping.h"
#include "input.h"
#include "profile.h"

extern Profile testProfile;

float valueSet(const char *title, float input, bool clamp = false, float clampMin = -255, float clampMax = 255);

void thrAdd(Profile& p);

void axsAdd(Profile& p);

void editMapping(Profile& p);

void testProfilev2();