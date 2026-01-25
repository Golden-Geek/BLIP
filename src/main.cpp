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
  // long startTime = millis();
  root.update();
  // long endTime = millis();
  // long duration = endTime - startTime;
  // DBG("Main loop took " + std::to_string(duration) + " ms");

  delay(1);
}
