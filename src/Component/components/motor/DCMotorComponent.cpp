#include "UnityIncludes.h"

void DCMotorComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(enPin);
    AddIntParamConfig(dir1Pin);
    AddIntParamConfig(dir2Pin);

    AddFloatParam(speed);
}

bool DCMotorComponent::initInternal()
{
    setupPins();
    return true;
}

void DCMotorComponent::updateInternal()
{
}

void DCMotorComponent::clearInternal()
{
}

void DCMotorComponent::setupPins()
{
    if (curPin != -1)
    {
        if (ledCAttached)
        {
            ledcDetach(enPin);
            ledCAttached = false;
        }
    }

    if (dir1Pin >= 0 && dir2Pin >= 0 && enPin >= 0)
    {
        pinMode(dir1Pin, OUTPUT);
        pinMode(dir2Pin, OUTPUT);

        bool result = ledcAttach(enPin, 5000, 10);
        if (!result)
            NDBG("Failed to attach pin " + std::to_string(enPin) + " to PWM for DC Motor");

        ledCAttached = result;
        curPin = enPin;
    }
}

void DCMotorComponent::paramValueChangedInternal(ParamInfo *paramInfo)
{
    void* param = paramInfo->ptr;
    if (param == &speed)
    {
        gpio_set_level(gpio_num_t(dir1Pin), speed > 0 ? HIGH : LOW);
        gpio_set_level(gpio_num_t(dir2Pin), speed > 0 ? LOW : HIGH);
        NDBG("Speed changed " + std::to_string(speed) + " / " + std::to_string(dir1Pin) + " :" + (speed > 0 ? "HIGH" : "LOW") + " / " + std::to_string(dir2Pin) + " :" + (speed > 0 ? "LOW" : "HIGH"));
        ledcWrite(pwmChannel, abs(speed) * 1024.0f);
    }
    else if (param == &enPin || param == &dir1Pin || param == &dir2Pin)
    {
        setupPins();
    }
}