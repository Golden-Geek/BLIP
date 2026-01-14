#include "UnityIncludes.h"
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
    AddIntParamConfig(feedbackInterval);
    AddFloatParamConfig(lowBatteryThreshold);

    AddFloatParam(batteryLevel);
    AddFloatParam(voltage);
    AddBoolParam(charging);

    AddIntParamConfig(shutdownChargeNoSignal);      // seconds, 0 = disabled
    AddIntParamConfig(shutdownChargeSignalTimeout); // seconds, 0 = disabled
};

bool BatteryComponent::initInternal()
{
    if (batteryPin >= 0)
    {
        pinMode(batteryPin, INPUT);
        analogRead(batteryPin);                        // read once to setup the ADC
        analogReadResolution(12);                      // 0..4095
        analogSetPinAttenuation(batteryPin, ADC_11db); // ~0–3.9 V range (good for 3.3 V)
    }

    if (chargePin >= 0)
    {
        if (chargePin == RX)
            NDBG("Battery charge pin " + String(chargePin) + " is used by serial component, handling specially.");
        else
            pinMode(chargePin, BATTERY_CHARGE_PIN_MODE);
    }

    for (int i = 0; i < BATTERY_AVERAGE_WINDOW; i++)
        values[i] = analogRead(batteryPin);

    return true;
}

void BatteryComponent::updateInternal()
{
    if (millis() < lastBatteryCheck + BATTERY_CHECK_INTERVAL)
        return;

    lastBatteryCheck = millis();

    readChargePin();
    readBatteryLevel();
    checkShouldAutoShutdown();
}

void BatteryComponent::clearInternal()
{
}

Color BatteryComponent::getBatteryColor()
{
    Color full(0, 255, 0);
    Color mid(255, 255, 0);
    Color low(255, 0, 0);

    Color result;

    float lowThreshold = 0.2f;
    float midThreshold = 0.6f;
    float fullThreshold = 0.95f;

    if (batteryLevel < midThreshold)
    {
        float t = constrain((batteryLevel - lowThreshold) / (midThreshold - lowThreshold), 0.f, 1.f);
        result = low.lerp(mid, t);
    }
    else
    {
        float t = constrain((batteryLevel - midThreshold) / (fullThreshold - midThreshold), 0.f, 1.f);
        result = mid.lerp(full, t);
    }

    return result;
}

void BatteryComponent::readChargePin()
{
    // Read the charging state
    if(chargePin < 0)
        return;
        
    if (chargePin == RX && !readChargePinOnNextCheck)
        return;

    readChargePinOnNextCheck = false;

    bool chVal = digitalRead(chargePin);
    if (chargePin == RX) // put back the data line to serial if we used RX pin
    {
        Serial.begin(115200);
    }

    if (BATTERY_CHARGE_PIN_INVERTED)
        chVal = !chVal;

    SetParam(charging, chVal);
}

void BatteryComponent::readBatteryLevel()
{
    if (batteryPin >= 0)
    {

#ifdef BATTERY_READ_MILLIVOLTS
        // Read the battery in milliamps
        float curV = analogReadMilliVolts(batteryPin) * BATTERY_READ_MILLIVOLTS_MULTIPLIER / 1000.0f; // Convert to volts
                                                                                                      // DBG("Battery millivolts  " + String(values[valuesIndex]));

#else
        float curV = analogRead(batteryPin);
#endif
        if (curV < 0.1f)
        {
            // DBG("Battery read failed, stopping there");
            timeAtLowBattery = 0; // reset low battery time
            return;
        }

        //  NDBG(String("Battery read: ") + String(curV));

        values[valuesIndex] = curV;
        valuesIndex = (valuesIndex + 1) % BATTERY_AVERAGE_WINDOW;
    }

    long currentTime = millis();

    int checkInterval = feedbackInterval > 0 ? feedbackInterval : 3; // if no feedback; still check every 3 seconds for updating level and low battery

    if (checkInterval > 0 && currentTime > lastBatterySet + checkInterval * 1000)
    {
        if (chargePin >= 0)
        {
            if (chargePin == RX)
            {
                Serial.end();
                pinMode(chargePin, BATTERY_CHARGE_PIN_MODE);
                readChargePinOnNextCheck = true; // Delay reading charge pin until next check to allow pinMode to take effect
            }
        }

        if (batteryPin > 0)
        {

            float sum = 0;
            for (int i = 0; i < BATTERY_AVERAGE_WINDOW; i++)
                sum += values[i];

            float val = sum / BATTERY_AVERAGE_WINDOW;

            // NDBG(String("Battery average: ") + String(val));

#ifdef BATTERY_READ_MILLIVOLTS
            float relVal = constrain((val - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE), 0.f, 1.f);
#else
            float relVal = constrain((val - rawMin) / (rawMax - rawMin), 0.f, 1.f);
#endif
            float volt = BATTERY_MIN_VOLTAGE + relVal * (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);

            SetParam(voltage, volt);

            // NDBG(String("Battery voltage: ") + String(voltage) + "V");

            SetParam(batteryLevel, relVal);
            lastBatterySet = currentTime;

            bool isLow = voltage < lowBatteryThreshold;
            if (!isLow || charging)
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
    }
}

void BatteryComponent::checkShouldAutoShutdown()
{
    if (RootComponent::instance->isShuttingDown())
        return;

#ifdef USE_BUTTON
    if (RootComponent::instance->buttons.count > 0)
    {
        if (RootComponent::instance->buttons.items[0]->wasPressedAtBoot)
            return; // do not shutdown if button was pressed at boot
    }
#endif

#ifdef USE_BATTERY
    if (BatteryComponent::instance->chargePin != -1 && !BatteryComponent::instance->charging)
        return; // only shutdown if charging
#endif

    bool gotSignal = RootComponent::instance->timeAtLastSignal != 0;

    if (shutdownChargeNoSignal > 0 && !gotSignal)
    {

        if (millis() > shutdownChargeNoSignal * 1000)
        {
            NDBG("No signal received since boot " + String(shutdownChargeNoSignal) + " seconds while charging, shutting down.");
            RootComponent::instance->shutdown();
            return;
        }
    }

    if (shutdownChargeSignalTimeout > 0)
    {
        if (millis() > RootComponent::instance->timeAtLastSignal + shutdownChargeSignalTimeout * 1000)
        {
            NDBG("No signal received for " + String(shutdownChargeSignalTimeout) + " seconds while charging, shutting down.");
            RootComponent::instance->shutdown();
            return;
        }
    }
}
