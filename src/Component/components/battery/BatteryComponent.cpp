#include "BatteryComponent.h"
ImplementSingleton(BatteryComponent);

void BatteryComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(batteryPin);
    AddIntParamConfig(chargePin);
#ifndef BATTERY_READ_MILLIVOLTS
    AddIntParamConfig(rawMin);
    AddIntParamConfig(rawMax);
#endif
    AddBoolParam(sendFeedback);
    AddFloatParamConfig(lowBatteryThreshold);

    AddFloatParam(batteryLevel);
    AddBoolParam(charging);
}

bool BatteryComponent::initInternal()
{
    if (batteryPin >= 0)
    {
        pinMode(batteryPin, INPUT);
        analogRead(batteryPin); // read once to setup the ADC
        analogSetPinAttenuation(batteryPin, ADC_0db);
    }

    if (chargePin >= 0)
        pinMode(chargePin, BATTERY_CHARGE_PIN_MODE);

    for (int i = 0; i < BATTERY_AVERAGE_WINDOW; i++)
        values[i] = analogRead(batteryPin);

    return true;
}

void BatteryComponent::updateInternal()
{
    if (millis() > lastBatteryCheck + BATTERY_CHECK_INTERVAL)
    {
        lastBatteryCheck = millis();
        if (chargePin >= 0)
        {
            // Read the charging state
            bool chVal = digitalRead(chargePin);
            // DBG("Charging state: " + String(chVal));
            SetParam(charging, chVal);
        }

        if (batteryPin >= 0)
        {
#ifdef BATTERY_READ_MILLIVOLTS
            // Read the battery in milliamps
            float curV = analogReadMilliVolts(batteryPin) * BATTERY_READ_MILLIVOLTS_MULTIPLIER / 1000.0f; // Convert to volts
                                                                                                          // DBG("Battery millivolts  " + String(values[valuesIndex]));
#else
            float curV = analogRead(batteryPin);
            // DBG("Battery raw value: " + String(values[valuesIndex]));
#endif
            if (curV < 0.1f)
            {
                DBG("Battery read failed, stopping there");
                timeAtLowBattery = 0; // reset low battery time
                return;
            }

            values[valuesIndex] = curV;
            valuesIndex++;
        }
    }

    if (valuesIndex >= BATTERY_AVERAGE_WINDOW)
    {
        valuesIndex = 0;

        if (batteryPin > 0)
        {

            float sum = 0;
            for (int i = 0; i < BATTERY_AVERAGE_WINDOW; i++)
                sum += values[i];

            float val = sum / BATTERY_AVERAGE_WINDOW;

#ifdef BATTERY_VOLTAGE_CUSTOM_FUNC
            float voltage = BATTERY_VOLTAGE_CUSTOM_FUNC(val);
#else
            const float maxVoltage = 4.2f;
            const float minVoltage = 3.5f;

#ifdef BATTERY_READ_MILLIVOLTS
            float voltage = val; // Convert to volts
#else
            float relVal = (val - rawMin) / (rawMax - rawMin);
            float voltage = minVoltage + relVal * (maxVoltage - minVoltage);
#endif
#endif
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

float BatteryComponent::getVoltage_c6(float val)
{
    return 2 * val / 16 / 1000.0;
}
