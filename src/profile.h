#pragma once
#include "global.h"
#include "input.h"

bool saveConfig(const char *path = "/default.json");

bool loadConfig(const char *path = "/default.json");

extern String configPath;
extern bool systemReset;

bool sysSave();

bool sysLoad();

std::vector<String> listProfiles();

void profileMenu();