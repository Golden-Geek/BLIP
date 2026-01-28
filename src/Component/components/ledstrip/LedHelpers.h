
#pragma once

#define RED_MILLIAMP 16
#define GREEN_MILLIAMP 11
#define BLUE_MILLIAMP 15
#define DARK_MILLIAMP 1

// NEO PIXEL / FAST LED CONVERSION
#ifdef LED_USE_FASTLED

#ifdef LED_MODEL_WS2816
#define LED_DEFAULT_TYPE WS2816
#elif defined LED_MODEL_SK9822
#define LED_DEFAULT_TYPE SK9822
#else
#define LED_DEFAULT_TYPE WS2812B
#endif

#else

// NEO PIXEL BUS DEFINES
#ifdef LED_MODEL_WS2816
#define NeoPixelMethod NeoWs2816Method
#define NeoPixelFeature NeoGrbWs2816Feature
#define NeoPixelColor Rgb48Color
#define NeoPixelColorDivider 1
#elif defined LED_MODEL_SK9822
#define NEOPIXEL_CLOCKED
#define NeoPixelMethod DotStarMethod            
#define NeoPixelFeature DotStarBgrFeature
#define NeoPixelColor RgbColor
#define NeoPixelColorDivider 256
#else
#define NeoPixelMethod NeoWs2812Method
#define NeoPixelFeature NeoBgrFeature
#define NeoPixelColor RgbColor
#define NeoPixelColorDivider 256
#endif // LED_MODELS
#endif // LED_USE_FASTLED

#ifndef LED_DEFAULT_CHANNELS
#define LED_DEFAULT_CHANNELS 3
#endif

#ifndef LED_DUPLICATE
#define LED_DUPLICATE 1
#endif

#ifndef LED_DEFAULT_MULTILED_MODE
#define LED_DEFAULT_MULTILED_MODE FullColor
#endif

#ifndef LED_DEFAULT_CORRECTION
#define LED_DEFAULT_CORRECTION true
#endif

#ifdef LEDSTRIP_FIXED_COUNT
#define LEDSTRIP_MAX_COUNT LEDSTRIP_FIXED_COUNT
#define LEDSTRIP_FIXED_MANAGER true
#else
#define LEDSTRIP_FIXED_MANAGER false
#define LEDSTRIP_MAX_COUNT 2
#endif

#ifdef LED_FIXED_COUNT
#define LED_DEFAULT_COUNT LED_FIXED_COUNT
#define LED_MAX_COUNT LED_FIXED_COUNT
#else
#ifndef LED_MAX_COUNT
#define LED_DEFAULT_COUNT 60
#endif
#ifndef LED_MAX_COUNT
#define LED_MAX_COUNT 300
#endif
#endif

#ifndef LED_DEFAULT_DATA_PIN
#define LED_DEFAULT_DATA_PIN 25
#endif

#ifndef LED_DEFAULT_CLK_PIN
#define LED_DEFAULT_CLK_PIN -1
#endif

#ifndef LED_DEFAULT_COLOR_ORDER
#ifdef LED_USE_FASTLED
#define LED_DEFAULT_COLOR_ORDER GRB
#else
#define LED_DEFAULT_COLOR_ORDER NEO_GRB
#endif
#endif

#ifndef LED_DEFAULT_EN_PIN
#define LED_DEFAULT_EN_PIN -1
#endif

#ifndef LED2_DEFAULT_TYPE
#define LED2_DEFAULT_TYPE LED_DEFAULT_TYPE
#endif

#ifndef LED2_DEFAULT_DATA_PIN
#define LED2_DEFAULT_DATA_PIN -1
#endif

#ifndef LED2_DEFAULT_CLK_PIN
#define LED2_DEFAULT_CLK_PIN -1
#endif

#ifndef LED2_DEFAULT_COLOR_ORDER
#define LED2_DEFAULT_COLOR_ORDER LED_DEFAULT_COLOR_ORDER
#endif

#ifndef LED2_DEFAULT_EN_PIN
#define LED2_DEFAULT_EN_PIN -1
#endif

#ifndef LED_DEFAULT_BRIGHTNESS
#define LED_DEFAULT_BRIGHTNESS .5f
#endif

#ifndef LED_DEFAULT_INVERT_DIRECTION
#define LED_DEFAULT_INVERT_DIRECTION false
#endif

#ifndef LED_DEFAULT_MAX_POWER
#define LED_DEFAULT_MAX_POWER 0
#endif

#ifndef LED_BRIGHTNESS_FACTOR
#define LED_BRIGHTNESS_FACTOR .5f
#endif

#ifndef LED_LEDS_PER_PIXEL
#define LED_LEDS_PER_PIXEL 1
#endif