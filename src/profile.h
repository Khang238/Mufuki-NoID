#pragma once
#include "global.h"
#include "input.h"
#include "mapping.h"
#include "menu.h"

bool saveProfile(const char* path, Profile& p);

bool loadProfile(const char* path, Profile& p);

void unpackProfile(Profile& p);

void packProfile(Profile& p);

extern String configPath;
extern String mappingPath;
extern bool systemReset;

extern Profile prf;
extern OutputState mos;

bool sysSave();

bool sysLoad();

std::vector<String> listFiles(const char* path = "/");

std::vector<String> listProfiles();

std::vector<String> listAnimations();

std::vector<String> listMacro();

void profileMenu();

void axsICfg(uint8_t& srce, float& im, float& ix, bool imuSrc);

void axsOCfg(uint8_t& dest, float& om, float& ox);

void thrAdd(Profile& p);

void axsAdd(Profile& p);

void editMapping(Profile& p);
