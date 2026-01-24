
#pragma once

#define RED_MILLIAMP 16
#define GREEN_MILLIAMP 11
#define BLUE_MILLIAMP 15
#define DARK_MILLIAMP 1


//NEO PIXEL - FAST LED CONVERSION
#ifdef LED_USE_FASTLED
#define NEO_RGB RGB
#define NEO_RBG RBG
#define NEO_GRB GRB
#define NEO_GBR GBR
#define NEO_BRG BRG
#else
#define RGB NEO_RGB
#define RBG NEO_RBG
#define GRB NEO_GRB
#define GBR NEO_GBR
#define BRG NEO_BRG
#endif



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

#ifndef LED_MAX_COUNT
#define LED_MAX_COUNT 100
#endif

#ifndef LEDSTRIP_MAX_COUNT
#define LEDSTRIP_MAX_COUNT 1
#endif

#ifdef LED_FIXED_COUNT
#define LED_DEFAULT_COUNT LED_FIXED_COUNT
#define LED_MAX_COUNT LED_FIXED_COUNT
#else
#ifndef LED_MAX_COUNT
#define LED_DEFAULT_COUNT 60
#endif
#ifndef LED_MAX_COUNT
#define LED_MAX_COUNT 1024
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