#pragma once

#ifndef DISTANCE_MAX_COUNT
#define DISTANCE_MAX_COUNT 2
#endif

#ifndef DISTANCE_DEFAULT_TRIGGER_PIN
#define DISTANCE_DEFAULT_TRIGGER_PIN -1
#endif

#ifndef DISTANCE_DEFAULT_ECHO_PIN
#define DISTANCE_DEFAULT_ECHO_PIN -1
#endif

#define TRIG_PULSE_DURATION 10  // 10 microseconds

DeclareComponent(DistanceSensor, "distance", )

    DeclareIntParam(trigPin, DISTANCE_DEFAULT_TRIGGER_PIN);
DeclareIntParam(echoPin, DISTANCE_DEFAULT_ECHO_PIN);
DeclareIntParam(updateRate, 20); // in Hz
DeclareIntParam(distanceMax, 100);
DeclareFloatParam(value, 0);

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

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void paramValueChangedInternal(void *param) override;

HandleSetParamInternalStart
    CheckAndSetParam(trigPin);
    CheckAndSetParam(echoPin);
    CheckAndSetParam(updateRate);
    CheckAndSetParam(distanceMax);
HandleSetParamInternalEnd;

CheckFeedbackParamInternalStart
    CheckAndSendParamFeedback(value);
CheckFeedbackParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(trigPin);
    FillSettingsParam(echoPin);
    FillSettingsParam(updateRate);
    FillSettingsParam(distanceMax);
FillSettingsInternalEnd

    FillOSCQueryInternalStart
        FillOSCQueryIntParam(trigPin);
    FillOSCQueryIntParam(echoPin);
    FillOSCQueryIntParam(updateRate);
    FillOSCQueryIntParam(distanceMax);
    FillOSCQueryRangeParamReadOnly(value, 0, 1);
FillOSCQueryInternalEnd

    EndDeclareComponent

    // Manager
    DeclareComponentManager(DistanceSensor, DISTANCE, distances, distance, DISTANCE_MAX_COUNT)
        EndDeclareComponent
