#pragma once
#include "global.h"
#include "input.h"
#include "profile.h"
#include "effect.h"
#include "bledev.h"

#define GRAPH_WIDTH 88
#define GRAPH_HEIGHT 32

extern float graphData[GRAPH_WIDTH];
extern int graphIndex;

void pushGraphValue(float val);

void drawGraph(int x, int y);

void calibMenu();

void inputMenu();

void filtMenu();

void effectMenu();

void displaySetting();

void mpuMenu();

void showDebug();

void deadCalib();

void waitAction(bool state);

void splScreen(const char* title, const char* t1, const char* t2, const char* btn = "btn", bool dobtn = true, bool waitKey = true);

void firstTimeSetup();

void otherMenu();

void about();

void wifiConnectScreen();

void wifiMenu();

void connectMenu();