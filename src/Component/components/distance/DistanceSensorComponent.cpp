#include "UnityIncludes.h"
#include "DistanceSensorComponent.h"

ImplementManagerSingleton(DistanceSensor);

void DistanceSensorComponent::setupInternal(JsonObject o)
{
    setCustomUpdateRate(60, o);

#ifdef DISTANCE_SENSOR_HCSR04
    AddIntParamConfig(trigPin);
    AddIntParamConfig(echoPin);
#elif defined(DISTANCE_SENSOR_VL53L0X)
    AddBoolParam(isConnected);
#endif

    AddIntParamConfig(distanceMax);
    AddIntParamConfig(debounceFrame);
    AddFloatParam(value);
    AddIntParamConfig(sendRate);

    for (int i = 0; i < DEBOUNCE_MAX_FRAMES; i++)
    {
        debounceBuffer[i] = 1.0f;
    }
}

bool DistanceSensorComponent::initInternal()
{
#ifdef DISTANCE_SENSOR_HCSR04
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
#elif defined(DISTANCE_SENSOR_VL53L0X)
    Wire.begin();
    sensor.setTimeout(500);
    initVL53L0X();
    sensor.startContinuous(1000 / updateRate);
#endif

    return true;
}

void DistanceSensorComponent::update()
{
    //Override update() to handle updateRate custom inside the sensors functions

    if(!enabled)
        return;

#ifdef DISTANCE_SENSOR_HCSR04
    updateHCSR04();
#elif defined(DISTANCE_SENSOR_VL53L0X)
    updateVL53L0X();
#endif
}

#ifdef DISTANCE_SENSOR_HCSR04
void DistanceSensorComponent::updateHCSR04()
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
#elif defined(DISTANCE_SENSOR_VL53L0X)

bool DistanceSensorComponent::initVL53L0X()
{
    if (!sensor.init())
    {
        NDBG("Failed to detect and initialize VL53L0X sensor!");
        SetParam(isConnected, false);
        return false;
    }

    NDBG("VL53L0X sensor initialized, starting continuous measurements.");
    SetParam(isConnected, true);
    return true;
}

void DistanceSensorComponent::updateVL53L0X()
{
    if (!isInit)
        return;

    if (!isConnected)
    {
        if (millis() - lastConnectTime > 5000)
        {
            initVL53L0X();
            lastConnectTime = millis();
        }
        return;
    }

    long currentTime = millis();
    if (currentTime - lastMeasurementTime < 1000 / updateRate)
        return; // Not time for the next measurement yet

    uint16_t range = 0;
    bool success = sensor.readRangeNoBlocking(range);

    if (!success)
    {
        // No new measurement available yet
        return;
    }

    lastMeasurementTime = currentTime;

    if (sensor.timeoutOccurred())
    {
        NDBG("VL53L0X sensor timeout!");
        SetParam(isConnected, false);
        return;
    }

    float distance = (float)range / 10.0f; // Convert mm to cm

    // NDBG("Distance: " +String(distance)+ " cm");

    if (distance > distanceMax)
        distance = distanceMax; // Cap to max range

    float val = distance / distanceMax;

    // Debounce logic
    int frames = min(debounceFrame, DEBOUNCE_MAX_FRAMES);

    debounceBuffer[debounceIndex] = val;
    debounceIndex = (debounceIndex + 1) % debounceFrame;

    float maxVal = 0.0f;
    for (int i = 0; i < frames; i++)
    {
        if (debounceBuffer[i] > maxVal)
            maxVal = debounceBuffer[i];
    }

    SetParam(value, maxVal);
}
#endif

void DistanceSensorComponent::paramValueChangedInternal(void *param)
{
#ifdef DISTANCE_SENSOR_HCSR04
    if (param == &trigPin)
    {
        pinMode(trigPin, OUTPUT);
    }
    else if (param == &echoPin)
    {
        pinMode(echoPin, INPUT);
    }
#endif

#ifdef DISTANCE_SENSOR_VL53L0X
    if (param == &updateRate)
    {
        if (isInit)
        {
            NDBG("Updating VL53L0X update rate to " + String(updateRate) + " Hz");
            sensor.stopContinuous();
            sensor.startContinuous(1000 / updateRate);
        }
    }
#endif
}