#include "UnityIncludes.h"

void IRComponent::setupInternal(JsonObject o)
{
    curPin1 = -1;
    ledCAttached1 = false;
    curPin2 = -1;
    ledCAttached2 = false;

    AddIntParamConfig(pin1);
    AddIntParamConfig(pin2);
    AddFloatParam(value);
    AddBoolParamConfig(keepValueOnReboot);
    setParamConfig(&value, keepValueOnReboot);

    prevValue = -1;
}

bool IRComponent::initInternal()
{
    setupPins();
    updatePins();

    return true;
}

void IRComponent::updateInternal()
{
}

void IRComponent::clearInternal()
{
}

void IRComponent::paramValueChangedInternal(void *param)
{

    if (param == &pin1 || param == &pin2)
        setupPins();
    if (param == &value)
        updatePins();
    if (param == &keepValueOnReboot)
        setParamConfig(&value, keepValueOnReboot);
}

void IRComponent::setupPins()
{
    if (curPin1 != -1 && ledCAttached1)
        ledcDetach(curPin1);
    if (curPin2 != -1 && ledCAttached2)
        ledcDetach(curPin2);

    curPin1 = pin1;
    curPin2 = pin2;

    if (curPin1 > 0)
    {
        bool result = ledcAttach(curPin1, 5000, 10);
        if (!result)
            NDBG("Failed to attach pin1 " + String(curPin1) + " to PWM");

        ledCAttached1 = result;
    }

    if (curPin2 > 0)
    {
        bool result = ledcAttach(curPin2, 5000, 10);
        if (!result)
            NDBG("Failed to attach pin2 " + String(curPin2) + " to PWM");

        ledCAttached2 = result;
    }

    prevValue = -1; // force update on next updatePins call
}

void IRComponent::updatePins()
{
    if (prevValue == value)
        return;

    uint32_t v = value * 1024;
    if (curPin1 > 0 && ledCAttached1)
        ledcWrite(curPin1, v);
    if (curPin2 > 0 && ledCAttached2)
        ledcWrite(curPin2, v);

    prevValue = value;
}