#include "UnityIncludes.h"
#include "DistanceSensorComponent.h"

ImplementManagerSingleton(DistanceSensor);

void DistanceSensorComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(trigPin);
    AddIntParamConfig(echoPin);
    AddIntParamConfig(updateRate);
    AddIntParamConfig(distanceMax);
    AddFloatParam(value);
}

bool DistanceSensorComponent::initInternal()
{
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    return true;
}

void DistanceSensorComponent::updateInternal()
{
    int currentEchoState = digitalRead(echoPin);

    // --- State Machine ---
    switch (currentState)
    {
    case IDLE:
        // Start a new measurement if enough time has passed since the last one
        if (millis() - stateStartTime >= max(1000 / updateRate, 60)) // Ensure at least 60ms between measurements
        {
            // Start Trigger sequence
            digitalWrite(trigPin, LOW); // Ensure low before pulse
            delayMicroseconds(2);       // Small delay for clean pulse
            digitalWrite(trigPin, HIGH);
            stateStartTime = micros(); // Record the start time of the trigger pulse
            currentState = TRIGGERING;
            RootComponent::instance->strips.items[0]->doNotUpdate = true;
        }
        break;

    case TRIGGERING:
        // Check if the 10 microsecond trigger pulse is complete
        if (micros() - stateStartTime >= TRIG_PULSE_DURATION)
        {
            digitalWrite(trigPin, LOW);
            currentState = WAITING_ECHO;
            // stateStartTime now marks the beginning of the wait for echo
        }
        break;

    case WAITING_ECHO:
        // Wait for the echo pin to go HIGH (start of the pulse)
        if (currentEchoState == HIGH)
        {
            pulseStart = micros(); // Record the start time of the echo pulse
            currentState = MEASURING;
        }
        // Timeout if the echo never starts.
        // Max duration is for round trip at distanceMax. Add a margin.
        else if (micros() - stateStartTime > (unsigned long)((distanceMax * 2.0f / 0.0343f) + 1000))
        {
            // Timeout: No echo received, reset state machine
            currentState = IDLE;
            stateStartTime = millis();
            RootComponent::instance->strips.items[0]->doNotUpdate = false;
        }
        break;

    case MEASURING:
        // Wait for the echo pin to go LOW (end of the pulse)
        if (currentEchoState == LOW && lastEchoState == HIGH)
        {
            pulseEnd = micros(); // Record the end time of the echo pulse
            currentState = COMPLETE;
        }
        // Timeout if the echo pulse is too long (object out of range or error)
        else if (micros() - pulseStart > (unsigned long)(distanceMax * 2.0f / 0.0343f))
        {
            // Timeout: pulse is longer than max possible, treat as max distance
            pulseEnd = pulseStart + (unsigned long)(distanceMax * 2.0f / 0.0343f);
            currentState = COMPLETE;
        }
        break;

    case COMPLETE:
        // Calculate and print the results
        float duration = (float)(pulseEnd - pulseStart);

        // Speed of sound in cm/microsecond at ~20°C: 0.0343 cm/µs
        // distance = (duration * 0.0343) / 2
        float distance = (duration * 0.0343) / 2.0f;

        // Clamp distance to max value
        if (distance > distanceMax)
            distance = distanceMax;

        SetParam(value, distance / distanceMax);

        stateStartTime = millis(); // Record completion time for the next IDLE check
        currentState = IDLE;
        RootComponent::instance->strips.items[0]->doNotUpdate = false;

        break;
    }

    // Update the static variable for the next iteration
    lastEchoState = currentEchoState;
}
void DistanceSensorComponent::paramValueChangedInternal(void *param)
{
    if (param == &trigPin)
    {
        pinMode(trigPin, OUTPUT);
    }
    else if (param == &echoPin)
    {
        pinMode(echoPin, INPUT);
    }
}