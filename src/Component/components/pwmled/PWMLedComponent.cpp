ImplementManagerSingleton(PWMLed);

bool PWMLedComponent::initInternal(JsonObject o)
{
    AddIntParamConfig(rPin);
    AddIntParamConfig(gPin);
    AddIntParamConfig(bPin);
    AddIntParamConfig(wPin);
    // AddColorParam(color);

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
        pwmChannels[i] = RootComponent::instance->getFirstAvailablePWMChannel();
        ledcSetup(pwmChannels[i], 5000, 10); // 0-1024 at a 5khz resolution
        ledcAttachPin(pins[i], pwmChannels[i]);
        RootComponent::availablePWMChannels[pwmChannels[i]] = false;
    }
}

void PWMLedComponent::updatePins()
{
    for (int i = 0; i < 4; i++)
    {
        ledcWrite(pwmChannels[i], color[i] * 1024);
    }
}