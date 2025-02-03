#include "UnityIncludes.h"
#include "PWMLedComponent.h"
ImplementManagerSingleton(PWMLed);

void PWMLedComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(rPin);
    AddIntParamConfig(gPin);
    AddIntParamConfig(bPin);
    AddIntParamConfig(wPin);
    AddIntParamConfig(whiteTemperature);
    AddBoolParamConfig(useAlpha);
    AddColorParam(color);
}

bool PWMLedComponent::initInternal()
{
    setupPins();
    updatePins();

#ifdef PWMLED_SHOW_INIT
    for (int i = 0; i < 100; i++)
    {
        float r = 0;
        float g = 1;
        float b = 1;
        // fade in out in 100 sine
        float a = sin(i / 100.0 * 2 * PI);
        setColor(r, g, b, a);
        delay(10);
    }
#endif

    return true;
}

void PWMLedComponent::updateInternal()
{

#ifdef PWMLED_SHOW_CONNECTION
#ifdef USE_WIFI
#ifdef USE_ESPNOW
    if (ESPNowComponent::instance->pairingMode != prevPairingMode)
    {
        if (ESPNowComponent::instance->pairingMode)
            setColor(0, 1, 1, 1);
        else
            setColor(0, 0, 0, 1);

        prevPairingMode = ESPNowComponent::instance->pairingMode;
        return;
    }
#else
    if (WifiComponent::instance->state != prevState)
    {
        switch (WifiComponent::instance->state)
        {
        case WifiComponent::Connected:
            setColor(0, 0, 0, 1);
            break;

        case WifiComponent::Connecting:
            setColor(0, 1, 1, 1);
            break;
        case WifiComponent::ConnectionError:
            setColor(1, 0, 0, 1);
            break;

        default:
            setColor(1, 0, 1, 1);
            break;
        }

        prevState = WifiComponent::instance->state;
        return;
    }
#endif
#endif
#endif

    updatePins();
}

void PWMLedComponent::clearInternal()
{
#ifdef PWMLED_SHOW_SHUTDOWN
    for (int i = 0; i <= 100; i++)
    {
        // fade red to black
        float r = 1;
        float g = 0;
        float b = 0;
        float a = 1 - i / 100.0;
        setColor(r, g, b, a);
        delay(5);
    }

    delay(50);
#endif
}

void PWMLedComponent::setupPins()
{
    const int pins[4]{rPin, gPin, bPin, wPin};
    for (int i = 0; i < 4; i++)
    {
        if (pins[i] == -1)
            continue;

#ifdef ARDUINO_NEW_VERSION
        ledcAttach(pins[i], 5000, 10); // pwmChannels[i]);
#else
        pwmChannels[i] = RootComponent::instance->getFirstAvailablePWMChannel();
        ledcSetup(pwmChannels[i], 5000, 10);    // 0-1024 at a 5khz resolution
        ledcAttachPin(pins[i], pwmChannels[i]); // pwmChannels[i]);
        RootComponent::availablePWMChannels[pwmChannels[i]] = false;
#endif
    }
}

void PWMLedComponent::updatePins()
{
    float alpha = useAlpha ? color[3] : 1;

    float r = color[0];
    float g = color[1];
    float b = color[2];

    float tr = r;
    float tg = g;
    float tb = b;
    float tw = 0;

    if (wPin != -1)
    {
        RGBToRGBW(r, g, b, tr, tg, tb, tw);
    }

    const int pins[4]{rPin, gPin, bPin, wPin};
    const float cols[4]{tr, tg, tb, tw};

    for (int i = 0; i < 4; i++)
    {
        if (pins[i] == -1)
            continue;

#ifdef ARDUINO_NEW_VERSION
        ledcWrite(pins[i], cols[i] * 1024 * alpha);
#else
        ledcWrite(pwmChannels[i], cols[i] * 1024 * alpha);
#endif
    }
}

void PWMLedComponent::paramValueChangedInternal(void *param)
{

    // DBG("Param changed " + getParamString(param));

    // if (param == &color)
    // {
    //     DBG("Color changed : " + getParamString(param));
    // }
}

void PWMLedComponent::setColor(float r, float g, float b, float a, bool show)
{
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
    if (show)
        updatePins();
}

void PWMLedComponent::RGBToRGBW(float r, float g, float b, float &rOut, float &gOut, float &bOut, float &wOut)
{
    float temperatureColor[3];
    // Calculate temperature color based on whiteTemperature
    float t = (10000.0f / whiteTemperature);
    float t2 = t * t;
    float t3 = t2 * t;

    if (whiteTemperature <= 6500)
    {
        temperatureColor[0] = 1.0f;
        temperatureColor[1] = 0.39008157876901960784f * log(whiteTemperature) - 0.63184144378862745098f;
        temperatureColor[2] = 0.54320678911019607843f * log(whiteTemperature) - 1.19625408914f;
    }
    else
    {
        temperatureColor[0] = 1.29293618606274509804f * pow(whiteTemperature - 6000, -0.1332047592f);
        temperatureColor[1] = 1.12989086089529411765f * pow(whiteTemperature - 6000, -0.0755148492f);
        temperatureColor[2] = 1.0f;
    }

    // Calculate all of the color's white values corrected taking into account the white color temperature.
    float wRed = r / temperatureColor[0];
    float wGreen = g / temperatureColor[1];
    float wBlue = b / temperatureColor[2];

    // Determine the smallest white value from above.
    float wMin = std::min(wRed, std::min(wGreen, wBlue));

    // Make the color with the smallest white value to be the output white value
    if (wMin == wRed)
        wOut = r;
    else if (wMin == wGreen)
        wOut = g;
    else
        wOut = b;

    // Calculate the output red, green and blue values, taking into account the white color temperature.
    rOut = r - wOut * temperatureColor[0];
    gOut = g - wOut * temperatureColor[1];
    bOut = b - wOut * temperatureColor[2];
}
