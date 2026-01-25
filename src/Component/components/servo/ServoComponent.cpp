#include "UnityIncludes.h"

void ServoComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(pin);
    AddFloatParamConfig(position);
}

bool ServoComponent::initInternal()
{
    if (pin > 0)
        servo.attach(pin);
    return true;
}

void ServoComponent::updateInternal()
{
}

void ServoComponent::clearInternal()
{
    servo.detach();
}

void ServoComponent::paramValueChangedInternal(void *param)
{
    if (param == &pin)
    {
        servo.detach();
        if (pin > 0)
        {
            servo.attach(pin);
        }
    }

    if (param == &position)
    {
        servo.write(position);
    }
}