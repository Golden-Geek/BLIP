
#pragma once

#define RED_MILLIAMP 16
#define GREEN_MILLIAMP 11
#define BLUE_MILLIAMP 15
#define DARK_MILLIAMP 1

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

#ifndef LED_DEFAULT_COUNT
#define LED_DEFAULT_COUNT 100
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
#define LED2_DEFAULT_DATA_PIN 17
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
#define LED_BRIGHTNESS_FACTOR 1.0f
#endif

#ifndef LED_LEDS_PER_PIXEL
#define LED_LEDS_PER_PIXEL 1
#endif