#pragma once

#ifndef DISTANCE_MAX_COUNT
#define DISTANCE_MAX_COUNT 2
#endif

#if not defined DISTANCE_SENSOR_HCSR04 && not defined DISTANCE_SENSOR_VL53L0X
#define DISTANCE_SENSOR_HCSR04
#endif

#ifdef DISTANCE_SENSOR_HCSR04
#ifndef DISTANCE_DEFAULT_TRIGGER_PIN
#define DISTANCE_DEFAULT_TRIGGER_PIN -1
#endif

#ifndef DISTANCE_DEFAULT_ECHO_PIN
#define DISTANCE_DEFAULT_ECHO_PIN -1
#endif

#define TRIG_PULSE_DURATION 10 // 10 microseconds
#elif defined(DISTANCE_SENSOR_VL53L0X)
#include <VL53L0X_mod.h>
#endif

DeclareComponent(DistanceSensor, "distance", )

#ifdef DISTANCE_SENSOR_HCSR04
    DeclareIntParam(trigPin, DISTANCE_DEFAULT_TRIGGER_PIN);
DeclareIntParam(echoPin, DISTANCE_DEFAULT_ECHO_PIN);
#elif defined(DISTANCE_SENSOR_VL53L0X)
    DeclareBoolParam(isConnected, false);
#endif

DeclareIntParam(updateRate, 20); // in Hz
DeclareIntParam(distanceMax, 100);
DeclareFloatParam(value, 0);
DeclareIntParam(sendRate, 0); // in Hz, 0 = send every update, -1 = never

#ifdef DISTANCE_SENSOR_HCSR04
// Variables for the state machine
enum SensorState
{
    IDLE,         // Ready to start a new measurement
    TRIGGERING,   // Sending the trigger pulse
    WAITING_ECHO, // Waiting for the echo pin to go HIGH
    MEASURING,    // Measuring the echo duration
    COMPLETE      // Measurement is complete
};

SensorState currentState = IDLE;
unsigned long stateStartTime = 0;
unsigned long pulseStart = 0;
unsigned long pulseEnd = 0;
int lastEchoState = LOW;

#elif defined(DISTANCE_SENSOR_VL53L0X)

unsigned long lastConnectTime = 0;
VL53L0X_mod sensor;
unsigned long lastMeasurementTime = 0;
unsigned long lastSendTime = 0;

#endif

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;

#ifdef DISTANCE_SENSOR_HCSR04
void updateHCSR04();
#elif defined(DISTANCE_SENSOR_VL53L0X)
bool initVL53L0X();
void updateVL53L0X();
#endif

void paramValueChangedInternal(void *param) override;

HandleSetParamInternalStart
#ifdef DISTANCE_SENSOR_HCSR04
    CheckAndSetParam(trigPin);
CheckAndSetParam(echoPin);
#endif
CheckAndSetParam(updateRate);
CheckAndSetParam(distanceMax);
CheckAndSetParam(sendRate);
HandleSetParamInternalEnd;

CheckFeedbackParamInternalStart;
CheckAndSendParamFeedback(isConnected);
if (sendRate < 0)
    return false;
if (sendRate > 0 && millis() - lastSendTime < (1000 / sendRate))
    return false;

lastSendTime = millis();
CheckAndSendParamFeedback(value);
CheckFeedbackParamInternalEnd;

FillSettingsInternalStart
#ifdef DISTANCE_SENSOR_HCSR04
    FillSettingsParam(trigPin);
FillSettingsParam(echoPin);
#endif
FillSettingsParam(updateRate);
FillSettingsParam(distanceMax);
FillSettingsParam(sendRate);
FillSettingsInternalEnd

    FillOSCQueryInternalStart
#ifdef DISTANCE_SENSOR_HCSR04
        FillOSCQueryIntParam(trigPin);
FillOSCQueryIntParam(echoPin);
#elif defined(DISTANCE_SENSOR_VL53L0X)
        FillOSCQueryBoolParamReadOnly(isConnected);
#endif
FillOSCQueryIntParam(updateRate);
FillOSCQueryIntParam(distanceMax);
FillOSCQueryRangeParamReadOnly(value, 0, 1);
FillOSCQueryIntParam(sendRate);
FillOSCQueryInternalEnd

    EndDeclareComponent

    // Manager
    DeclareComponentManager(DistanceSensor, DISTANCE, distances, distance, DISTANCE_MAX_COUNT)
        EndDeclareComponent
