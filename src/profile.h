#pragma once
#include "global.h"
#include "input.h"
#include "mapping.h"

bool saveConfig(const char *path);

bool loadConfig(const char *path);

bool saveProfile(const char* path, const Profile& p);

bool loadProfile(const char* path, Profile& p);

extern String configPath;
extern bool systemReset;

bool sysSave();

bool sysLoad();

std::vector<String> listProfiles();

void profileMenu();