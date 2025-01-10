#include <Arduino.h>

#define MAIN_INCLUDE
#include "UnityIncludes.h"

extern "C" {
#include <esp32/spiram.h>
#include <esp32/himem.h>
}

RootComponent root("root");

void setup()
{
	root.setup();
	root.init();
    DBG("Device is init");
}

void loop()
{
	root.update();
}