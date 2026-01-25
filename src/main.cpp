#include <Arduino.h>

#include "UnityIncludes.h"
RootComponent root("root");

void setup()
{
  root.setup();
  root.init();
  DBG("[Main] Device is init");
}

void loop()
{
  root.update();

  delay(1);
}
