#pragma once

#include "Arduino.h"

#include <Wire.h>

// Declarations
#define ARDUINOJSON_USE_LONG_LONG 0
#define ARDUINOJSON_USE_DOUBLE 0

#define SHUTDOWN_ANIM_TIME 1

#include <ArduinoJson.h>
#include <FS.h>
#include <SPI.h>
#include <arduino-timer.h>
#include <map>
#include <ostream>
#ifdef USE_ARTNET
#include <ArtnetWifi.h>
#endif

#ifdef USE_M5UNIFIED
#include <M5Unified.h>
#endif

// Firmware
#include "Common/Helpers.h"
#include "Common/BoardDefines.h"
#include "Common/var.h"
#include "Common/color.h"
#include "Common/EventBroadcaster.h"
#include "Common/Settings.h"
#include "Common/StringHelpers.h"
#include "Common/CommonListeners.h"

#ifdef USE_SCRIPT // needs pre declaration to be used by Component
#include <wasm3.h>
#include <m3_env.h>
#include <SimplexNoise.h>
#include "Common/script/Script.h"
#include "Common/script/wasmFunctions.h"
#endif

#include "Component/ComponentEvent.h"
#include "Component/Component.h"

#ifdef USE_FILES

#if defined FILES_TYPE_SD
#elif defined FILES_TYPE_FLASH
#else
#ifndef FILES_TYPE_LittleFS
#define FILES_TYPE_LittleFS
#endif
#endif

#include "FS.h"
#include "LittleFS.h"

#include "Component/components/files/FilesComponent.h"
#endif

#ifdef USE_WIFI
#include "Component/components/wifi/WifiComponent.h"
#endif

#ifdef USE_OSC
#include <OSCMessage.h>
#include <ESPmDNS.h>
#include "Component/components/communication/osc/OSCComponent.h"
#endif

#ifdef USE_SERIAL
#include "Component/components/communication/serial/SerialComponent.h"
#endif

#ifdef USE_ESPNOW
#include <esp_now.h>
#include "Component/components/communication/espnow/ESPNowComponent.h"
#endif

#ifdef USE_SERVER

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Component/components/communication/server/WebServerComponent.h"
#endif

#ifdef USE_OTA
#include "Update.h"
#endif

#include "Component/components/communication/CommunicationComponent.h"

#ifdef USE_SCRIPT
#include "Component/components/script/ScriptComponent.h"
#endif

#if defined USE_IO || defined USE_BUTTON
#include "Component/components/io/IOComponent.h"
#endif

#ifdef USE_BUTTON
#include "Component/components/io/button/ButtonComponent.h"
#endif

#ifdef USE_IR
#include "Component/components/ir/IRComponent.h"
#endif

#ifdef USE_BATTERY
#include "Component/components/battery/BatteryComponent.h"
#endif

#ifdef USE_MOTION

#ifndef IMU_TYPE
#define IMU_TYPE BNO055
#endif

#ifdef IMU_TYPE_BNO055
#include <utility/vector.h>
#include <utility/matrix.h>
#include <utility/quaternion.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#elif defined IMU_TYPE_M5MPU
#endif
#include "Component/components/motion/MotionComponent.h"
#endif

#if defined USE_DMX || defined USE_ARTNET
#include "Component/components/dmx/DMXReceiverComponent.h"
#endif

#ifdef USE_LEDSTRIP
#ifdef LED_USE_FASTLED
#include <FastLED.h>
#else
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#endif
#include "Component/components/ledstrip/LedHelpers.h"

#include "Component/components/ledstrip/Layer/LedStripLayer.h"
#include "Component/components/ledstrip/Layer/layers/playback/LedStripPlaybackLayer.h"
#include "Component/components/ledstrip/Layer/layers/system/LedStripSystemLayer.h"

#if defined USE_DMX || defined USE_ARTNET
#include "Component/components/ledstrip/Layer/layers/stream/LedStripStreamLayer.h"
#endif

#include "Component/components/ledstrip/FXComponent.h"
#include "Component/components/ledstrip/LedStripComponent.h"
#endif // LEDSTRIP

#ifdef USE_PWMLED
#include "Component/components/pwmled/PWMLedComponent.h"
#endif

#ifdef USE_SEQUENCE
#include "Component/components/sequence/SequenceComponent.h"
#endif

#ifdef USE_SERVO
#include <ESP32Servo.h>
#include "Component/components/servo/ServoComponent.h"
#endif

#ifdef USE_STEPPER
#include <FastAccelStepper.h>
#include "Component/components/stepper/StepperComponent.h"
#endif

#ifdef USE_DC_MOTOR
#include "Component/components/motor/DCMotorComponent.h"
#endif

#ifdef USE_DISTANCE
#include "Component/components/distance/DistanceSensorComponent.h"
#endif

#ifdef USE_DIPSWITCH
#include "Component/components/dipswitch/DIPSwitchComponent.h"
#endif

#ifdef USE_BEHAVIOUR
#include "Component/components/behaviour/BehaviourComponent.h"
#endif

#ifdef USE_DISPLAY
#include "Component/components/display/DisplayComponent.h"
#endif

#ifdef USE_DUMMY
#include "Component/components/dummy/DummyComponent.h"
#endif

#include "Component/components/settings/SettingsComponent.h"

#include "RootComponent.h"