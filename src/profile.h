#pragma once
#include "global.h"
#include "input.h"
#include "mapping.h"
#include "menu.h"

bool saveConfig(const char *path);

bool loadConfig(const char *path);

bool saveProfile(const char* path, Profile& p);

bool loadProfile(const char* path, Profile& p);

void unpackProfile(Profile& p);

void packProfile(Profile& p);

bool saveProfileVer(const char* path, Profile& p);

int getProfileVersion(const char* path);

bool loadProfileVer(const char* path, Profile& p);

extern String configPath;
extern String mappingPath;
extern bool systemReset;

extern Profile prf;
extern OutputState mos;

bool sysSave();

bool sysLoad();

std::vector<String> listProfiles();

void profileMenu();

void thrAdd(Profile& p);

void axsAdd(Profile& p);

void editMapping(Profile& p);
