#include "BatteryComponent.h"
ImplementSingleton(BatteryComponent);

bool BatteryComponent::initInternal(JsonObject o)
{
    AddIntParamConfig(batteryPin);
    AddIntParamConfig(chargePin);
    AddIntParamConfig(rawMin);
    AddIntParamConfig(rawMax);
    AddBoolParam(sendFeedback);
    AddFloatParamConfig(lowBatteryThreshold);

    AddFloatParam(batteryLevel);
    AddBoolParam(charging);

    if (batteryPin > 0)
    {
        pinMode(batteryPin, INPUT);
        analogSetPinAttenuation(batteryPin, ADC_0db);
    }

    if (chargePin > 0)
        pinMode(chargePin, INPUT);

    for (int i = 0; i < BATTERY_AVERAGE_WINDOW; i++)
        values[i] = analogRead(batteryPin);

    return true;
}

void BatteryComponent::updateInternal()
{
    if (millis() > lastBatteryCheck + BATTERY_CHECK_INTERVAL)
    {

        if (batteryPin > 0)
        {

            values[valuesIndex] = analogRead(batteryPin);
            valuesIndex++;
        }

        lastBatteryCheck = millis();
    }

    if (valuesIndex >= BATTERY_AVERAGE_WINDOW)
    {
        valuesIndex = 0;

        if (chargePin > 0)
        {
            SetParam(charging, (digitalRead(chargePin))); // measuredVal == 0;
        }

        if (batteryPin > 0)
        {

            float sum = 0;
            for (int i = 0; i < BATTERY_AVERAGE_WINDOW; i++)
                sum += values[i];

            float val = sum / BATTERY_AVERAGE_WINDOW;

            const float maxVoltage = 4.2f;
            const float minVoltage = 3.5f;

            float relVal = (val - rawMin) / (rawMax - rawMin);
            float voltage = minVoltage + relVal * (maxVoltage - minVoltage);

            SetParam(batteryLevel, voltage);

            bool isLow = batteryLevel < lowBatteryThreshold;
            if (!isLow)
                timeAtLowBattery = 0;
            else
            {
                if (timeAtLowBattery == 0)
                    timeAtLowBattery = millis();
                else if (millis() > timeAtLowBattery + 10000) // 10 seconds
                {
                    sendEvent(CriticalBattery);
                }
            }
        }

        lastBatterySet = millis();
    }
}

void BatteryComponent::clearInternal()
{
}

Color BatteryComponent::getBatteryColor()
{
    Color full(0, 255, 0);
    Color mid(255, 255, 0);
    Color low(255, 0, 0);

    float relBattery = (batteryLevel - lowBatteryThreshold) / (4.2f - lowBatteryThreshold);
    relBattery = constrain(relBattery, 0, 1);

    Color result;
    if (relBattery < .5f)
        result = low.lerp(mid, relBattery * 2);
    else
        result = mid.lerp(full, (relBattery - .5f) * 2);

    return result;
}