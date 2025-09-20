#include <Arduino.h>
#include "UnityIncludes.h"

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