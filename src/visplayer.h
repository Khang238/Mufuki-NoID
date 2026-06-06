// visplayer.h
#pragma once
#include "global.h"

bool visLoad(const char* path);   // load file vào LittleFS handle
void visPlay();                    // gọi trong display task, tự rate-limit theo fps
void visStop();
bool visIsPlaying();