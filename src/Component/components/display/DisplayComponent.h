
#pragma once

#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE M5StickC
#endif

#if DISPLAY_TYPE == M5StickC
#include <M5GFX.h>
#endif

#ifndef DISPLAY_DEFAULT_TEXT_SIZE
#define DISPLAY_DEFAULT_TEXT_SIZE 10
#endif

#ifndef DISPLAY_LOG_MAX_LINES
#define DISPLAY_LOG_MAX_LINES 5
#endif

class DisplayComponent : public IOComponent
{
public:
    DisplayComponent(const std::string &name = "display", bool _enabled = false) : IOComponent(name, _enabled), canvas(&display) {}

    enum DisplayType { M5StickC };

std::string logLines[DISPLAY_LOG_MAX_LINES];
int logLineIndex = 0;

bool shouldUpdateDisplay = false;

#if DISPLAY_TYPE == M5StickC
M5GFX display;
M5Canvas canvas;
#endif

bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

#if DISPLAY_TYPE == M5StickC
void initM5StickC();
#endif

void setDisplayText(const std::string &text, int x = 0, int y = 0, int size = DISPLAY_DEFAULT_TEXT_SIZE);
void log(const std::string &text);
void logWarning(const std::string &text);
void logError(const std::string &text);
void clearDisplay();

bool handleCommandInternal(const std::string &command, var *data, int numData) override;



};