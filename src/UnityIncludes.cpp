
#include "UnityIncludes.h"

#include "Common/Settings.cpp"
#include "Common/StringHelpers.cpp"



#include "Component/ComponentEvent.cpp"
#include "Component/Component.cpp"

#include "Component/components/settings/SettingsComponent.cpp"

//SD shit
#ifdef USE_FILES
// #include "../lib/SD/src/SD.cpp" //really weird
// #include "../lib/SD/src/sd_diskio.cpp" //really weird
#include "Component/components/files/FilesComponent.cpp"
#endif


#ifdef USE_WIFI
#include "Component/components/wifi/WifiComponent.cpp"
#endif


#ifdef USE_OSC
#include "Component/components/communication/osc/OSCComponent.cpp"
#endif

#ifdef USE_SERIAL
#include "Component/components/communication/serial/SerialComponent.cpp"
#endif

#ifdef USE_ESPNOW
#include "Component/components/communication/espnow/ESPNowComponent.cpp"
#endif

#include "Component/components/communication/CommunicationComponent.cpp"


#ifdef USE_SERVER
#include "Component/components/server/WebServerComponent.cpp"
#endif

#ifdef USE_SCRIPT
#include "Common/script/Script.cpp"
#include "Common/script/utf16.cpp"
#include "Common/script/wasmFunctions.cpp"
#include "Component/components/script/ScriptComponent.cpp"
#endif


#ifdef USE_IO
#include "Component/components/io/IOComponent.cpp"
#ifdef USE_BUTTON
#include "Component/components/io/button/ButtonComponent.cpp"
#endif
#endif

#ifdef USE_BATTERY
#include "Component/components/battery/BatteryComponent.cpp"
#endif

#ifdef USE_MOTION
#include "Component/components/motion/MotionComponent.cpp"
#endif

#if defined USE_STREAMING
#include "Component/components/ledstream/LedStreamReceiver.cpp"
#endif

#ifdef USE_LEDSTRIP
#include "Component/components/ledstrip/Layer/LedStripLayer.cpp"
#include "Component/components/ledstrip/Layer/layers/playback/LedStripPlaybackLayer.cpp"
#include "Component/components/ledstrip/Layer/layers/system/LedStripSystemLayer.cpp"
#ifdef USE_STREAMING
#include "Component/components/ledstrip/Layer/layers/stream/LedStripStreamLayer.cpp"
#endif

#include "Component/components/ledstrip/FXComponent.cpp"

#include "Component/components/ledstrip/LedStripComponent.cpp"
#endif

#ifdef USE_PWMLED
#include "Component/components/pwmled/PWMLedComponent.cpp"
#endif

#ifdef USE_SEQUENCE
#include "Component/components/sequence/SequenceComponent.cpp"
#endif

#ifdef USE_SERVO
#include "Component/components/servo/ServoComponent.cpp"
#endif

#ifdef USE_STEPPER
#include "Component/components/stepper/StepperComponent.cpp"
#endif

#ifdef USE_DC_MOTOR
#include "Component/components/motor/DCMotorComponent.cpp"
#endif

#ifdef USE_BEHAVIOUR
#include "Component/components/behaviour/BehaviourComponent.cpp"
#endif


#ifdef USE_DISPLAY
#include "Component/components/display/DisplayComponent.cpp"
#endif

#ifdef USE_DUMMY
#include "Component/components/dummy/DummyComponent.cpp"
#endif