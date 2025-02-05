#include "UnityIncludes.h"

ImplementManagerSingleton(PWMLed);

void PWMLedComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(rPin);
    AddIntParamConfig(gPin);
    AddIntParamConfig(bPin);
    AddIntParamConfig(wPin);
    // AddIntParamConfig(whiteTemperature);
    AddBoolParamConfig(useAlpha);
    AddBoolParamConfig(rgbwMode);
    AddColorParam(color);

#ifdef PWMLED_USE_STREAMING
    AddIntParamConfig(universe);
    AddIntParamConfig(ledIndex);
#endif
}

bool PWMLedComponent::initInternal()
{
#ifdef USE_STREAMING
    LedStreamReceiverComponent::instance->registerStreamListener(this);
#endif

    setupPins();
    updatePins();

#ifdef PWMLED_SHOW_INIT

    float r = 0;
    float g = .1f;
    float b = .1f;

#ifdef USE_ESPNOW
    if (ESPNowComponent::instance->enabled)
    {
        r = .1f;
        g = 0;
        b = .1f;
    }
#endif

    for (int i = 0; i < 100; i++)
    {
        // fade in out in 100 sine
        float a = (cos(PI + i * PI * 2 / 100.0) + .5f) * .5f;
        setColor(r, g, b, a);
        delay(5);
    }
#endif

    return true;
}

void PWMLedComponent::updateInternal()
{

    bool espNowMode = false;
#ifdef USE_ESPNOW
    espNowMode = ESPNowComponent::instance->enabled;
#endif

#ifdef PWMLED_SHOW_CONNECTION
#ifdef USE_WIFI
    if (espNowMode)
    {
#ifdef USE_ESPNOW
        if (ESPNowComponent::instance->enabled)
        {
            if (ESPNowComponent::instance->pairingMode)
            {
                setColor(.1f, 0, .1f, (sin(millis() / 300.0) + .5f) * .1f + .1f);
                prevPairingMode = true;
                return;
            }

            if (ESPNowComponent::instance->pairingMode != prevPairingMode)
            {
                if (!ESPNowComponent::instance->pairingMode)
                {
                    setColor(0, 0, 0, 1);
                    prevPairingMode = false;
                }
            }
        }
#endif
    }
    else
    {
        if (WifiComponent::instance->state != prevState)
        {
            switch (WifiComponent::instance->state)
            {
            case WifiComponent::Connected:
                setColor(.02f, .1f, 0, 1);
                delay(100);
                setColor(0, 0, 0, 1);
                break;

            case WifiComponent::Connecting:
                setColor(0, .05f, .05f, 1);
                break;
            case WifiComponent::ConnectionError:
                setColor(.05f, 0, 0, 1);
                break;

            default:
                setColor(.05f, 0, .05f, 1);
                break;
            }

            prevState = WifiComponent::instance->state;
            return;
        }
    }
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

#ifdef USE_STREAMING
    if (LedStreamReceiverComponent::instance != nullptr)
        LedStreamReceiverComponent::instance->unregisterStreamListener(this);
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
        // NDBG("Setting up pin " + String(pins[i]));
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

    if (wPin != -1 && rgbwMode)
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
    // Define the approximate RGB equivalent of your white LED
    const float warmWhiteR = 255.0f;
    const float warmWhiteG = 88.0f;
    const float warmWhiteB = 0.0f;

    // Normalize the white LED RGB components
    float wFactorR = warmWhiteR / 255.0f;
    float wFactorG = warmWhiteG / 255.0f;
    float wFactorB = warmWhiteB / 255.0f;

    // Compute the white component as the minimum of the color scaled to match the warm white LED
    float wRed = r / wFactorR;
    float wGreen = g / wFactorG;
    float wBlue = b / wFactorB;

    // Find the smallest contribution
    float wMin = std::min({wRed, wGreen, wBlue});
    wOut = std::max(0.0f, wMin);

    // Subtract the white component from the original RGB values
    rOut = r - (wOut * wFactorR);
    gOut = g - (wOut * wFactorG);
    bOut = b - (wOut * wFactorB);
}

void PWMLedComponent::onLedStreamReceived(uint16_t dmxUniverse, const uint8_t *data, uint16_t len)
{
    if(RootComponent::instance->isShuttingDown() || !RootComponent::instance->isInit)
        return;

    // NDBG("Received stream data for universe " + String(dmxUniverse) + " with " + String(len) + " bytes");

    int count = len / (useAlpha ? 4 : 3);
    int maxCount = useAlpha ? 128 : 170;
    int start = (dmxUniverse - universe) * maxCount - ledIndex;

    int iStart = start < 0 ? -start : 0;
    if (iStart < count)
    {
        float r = data[iStart * (useAlpha ? 4 : 3)] / 255.0f;
        float g = data[iStart * (useAlpha ? 4 : 3) + 1] / 255.0f;
        float b = data[iStart * (useAlpha ? 4 : 3) + 2] / 255.0f;
        float a = useAlpha ? data[iStart * 4 + 3] / 255.0f : 1.0f;

        setColor(r, g, b, a, true);
    }
}
