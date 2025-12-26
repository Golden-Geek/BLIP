#include <Arduino.h>

// #define TESTING_MODE

#ifndef TESTING_MODE
#include "UnityIncludes.h"
RootComponent root("root");
#else
  #include <FastLED.h>
  #define NUM_LEDS 100
  CRGB leds[NUM_LEDS];
  float pointPos = 0.0;
  void testLoop();
  void testSetup();
#endif


void setup()
{
  #ifndef TESTING_MODE
  root.setup();
  root.init(); 
  DBG("[Main] Device is init");
  #endif

  #ifdef TESTING_MODE
  testSetup();
  #endif
}

void loop()
{
  #ifndef TESTING_MODE
  root.update();
  #endif

  #ifdef TESTING_MODE
  testLoop();
  #endif
}



// RAW TESTING MODE

#ifdef TESTING_MODE
void testSetup()
{
  // Serial.begin(115200);
  // Serial.println("[Main] Testing mode setup");
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  FastLED.addLeds<WS2812B, 25, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(40);
  FastLED.clear();
}

void testLoop()
{
  static uint32_t lastUpdate = 0;
  static uint8_t hue = 0;

  if (millis() - lastUpdate > 10)
  {
    lastUpdate = millis();
    pointPos = fmodf(pointPos + 0.005, 1.0);
    // for (int i = 0; i < NUM_LEDS; i++)
    // {
    //   float dist = fabsf(((float)i / (float)NUM_LEDS) - pointPos);
    //   float distMap = 1.0 - (dist / 0.1f);
    //   distMap = min(max(distMap, 0.0f), 1.0f);
    //   //with fade
    //   uint8_t brightness = (uint8_t)(200.0 * distMap) + 50;
    //   leds[i] = CHSV(hue, 255, brightness);
    // }

    FastLED.clear();
    int pos = (int)(pointPos * (float)NUM_LEDS);
    leds[pos] = CHSV(hue, 255, 255);
    if(pos > 0)
      leds[pos - 1] = CHSV(hue, 255, 100);
    if(pos < NUM_LEDS - 1)
      leds[pos + 1] = CHSV(hue, 255, 100);

    FastLED.show();
    hue++;
  }
}
#endif

