#include "PWMLedComponent.h"
ImplementManagerSingleton(PWMLed);

void PWMLedComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(rPin);
    AddIntParamConfig(gPin);
    AddIntParamConfig(bPin);
    AddIntParamConfig(wPin);
    AddColorParam(color);
}

bool PWMLedComponent::initInternal()
{
    setupPins();
    updatePins();

    return true;
}

void PWMLedComponent::updateInternal()
{
    updatePins();
}

void PWMLedComponent::clearInternal()
{
}

void PWMLedComponent::setupPins()
{
    const int pins[4]{rPin, gPin, bPin, wPin};
    for (int i = 0; i < 4; i++)
    {
        if (pins[i] == -1)
            continue;
        pwmChannels[i] = RootComponent::instance->getFirstAvailablePWMChannel();
        // ledcSetup(pwmChannels[i], 5000, 10); // 0-1024 at a 5khz resolution
        ledcAttach(pins[i], 5000, 10); // pwmChannels[i]);
        RootComponent::availablePWMChannels[pwmChannels[i]] = false;
    }
}

void PWMLedComponent::updatePins()
{
    const int pins[4]{rPin, gPin, bPin, wPin};
    for (int i = 0; i < 4; i++)
    {
        if (pins[i] == -1)
            continue;
        ledcWrite(pins[i], color[i] * 1024);
    }
}
void PWMLedComponent::paramValueChangedInternal(void *param)
{
    
    DBG("Param changed " + getParamString(param));
    
    if (param == &color)
    {
        DBG("Color changed : " + getParamString(param));
    }
}