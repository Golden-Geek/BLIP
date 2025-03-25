#include "UnityIncludes.h"

void LedStripSystemLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);
    blendMode = BlendMode::Alpha;
}

void LedStripSystemLayer::updateInternal()
{

    clearColors();
    updateConnectionStatus();
    updateShutdown();
}

void LedStripSystemLayer::clearInternal()
{
    clearColors();
}

void LedStripSystemLayer::updateConnectionStatus()
{
    
    if (WifiComponent::instance == NULL)
        return;

    int numColors = strip->numColors;

    if (RootComponent::instance->testMode)
    {
        int numTest = 0;
#ifdef USE_MOTION
        numTest++;
#endif

#ifdef USE_FILES
        numTest++;
#endif

        int curTest = 0;
#ifdef USE_MOTION

        fillAll(Color(0, 0, 0)); // clear strip

        Color c = RootComponent::instance->motion.isInit ? Color(50, 255, 0) : Color(255, 0, 120);
        point(c, curTest++ * 1.0f / std::max(numTest - 1.0f, .5f), .05f, false);
#endif

#ifdef USE_FILES
        Color c2 = RootComponent::instance->files.isInit ? Color(50, 255, 0) : Color(255, 0, 120);
        point(c2, curTest++ * 1.0f / std::max(numTest - 1.0f, .5f), .05f, false);
#endif

        return;
    }

#ifdef USE_ESPNOW

    // DBG("Update !" + String(ESPNowComponent::instance->pairingMode) + " / " + String(ESPNowComponent::instance->bridgeInit));
    if (ESPNowComponent::instance->pairingMode || !ESPNowComponent::instance->bridgeInit)
    {
        if (ESPNowComponent::instance->pairingMode)
        {
            if (ESPNowComponent::instance->bridgeInit)
                fillAll(Color(50, 255, 0));
            else
                fillAll(Color(0, 120, 250));
        }
        else
            fillAll(Color(250, 50, 0));
    }

    // Color c = ESPNowComponent::instance->bridgeInit ? Color(50, 255, 0) : Color(255, 0, 120));
    // if (ESPNowComponent::instance->pairingMode && !ESPNowComponent::instance->bridgeInit) c = Color(0,120,250);

    // if (!ESPNowComponent::instance->lastReceiveTime == 0)
    // {
    //     // float pulseSpeed = 0.5; // Adjust this value to change the pace
    //     // float pulse = (sin(millis() * pulseSpeed / 1000.0f * PI * 2) * 0.5f + 0.5f) * 0.15f + 0.05f;
    //     // fillAll(Color(0, 0, 0, 255)); // clear strip
    //     // point(c, .5f, pulse, false);
    // }
    // else
    // {
    //     float rad = max(1 - (millis() - ESPNowComponent::instance->lastReceiveTime) * 2 / 1000.0f, 0.f) * .3f;
    //     fillAll(Color(0, 0, 0, 255)); // clear strip
    //     point(c, .5f, rad, false);
    // }

    return;
#endif

    float relT = (millis() - WifiComponent::instance->timeAtStateChange) / 1000.0f;
    const float animTime = 1.0f;

    WifiComponent::ConnectionState connectionState = WifiComponent::instance->state;

    // fillRange(Color(255, 255, 0), .25, .5f, false); // clear strip

    if (connectionState != WifiComponent::Connecting && relT > animTime)
        return;

#ifdef USE_BATTERY
    Color color = BatteryComponent::instance->getBatteryColor();
#else
    Color color = Color(100, 100, 100);
#endif

    // NDBG("Wifi status : " + String(connectionState) + " " + String(relT));

    // default behavior (connecting) on which we will add animation for connected behavior
    float t = (millis() - RootComponent::instance->timeAtStart) / 1000.0f;
    float pos = cos((t + PI / 2 + .2f) * 5) * .5f + .5f;

    if (strip->invertStrip)
        pos = 1 - pos;

    float radius = .3 - (cos(pos * PI * 2) * .5f + .5f) * .25f;
    float alpha = 1;

    if (connectionState != WifiComponent::Connecting)
    {
        Color targetColor;

        switch (connectionState)
        {
        case WifiComponent::ConnectionState::Connected:
            targetColor = Color(50, 255, 0);
            break;
        case WifiComponent::ConnectionError:
            targetColor = Color(255, 55, 0);
            break;
        case WifiComponent::Disabled:
            targetColor = Color(255, 0, 255);
            break;
        case WifiComponent::Hotspot:
            targetColor = Color(255, 255, 0);
            break;

        default:
            break;
        }

        float blendFac = min(relT * 2, 1.f);
        alpha = constrain(2 * (1 - relT / animTime), 0, 1);
        color = color.lerp(targetColor, blendFac).withMultipliedAlpha(alpha);

        radius = max(radius, relT * 3 / animTime); // increase radius to 1 in one second
    }
    else
    {
        // DBG("Rel T " + String(relT));
        color = color.withMultipliedAlpha(constrain((relT - .05f) * 1 / animTime, 0, 1));
    }

    fillAll(Color(0, 0, 0, alpha * 255)); // clear strip
    point(color, pos, radius, false);
}

void LedStripSystemLayer::updateShutdown()
{
    if (!RootComponent::instance->isShuttingDown())
        return;

    float relT = (millis() - RootComponent::instance->timeAtShutdown) / 1000.0f;
    const float animTime = 1.0f;

    float t = relT / animTime;

#ifdef USE_BATTERY
    Color c = BatteryComponent::instance->getBatteryColor();
#else
    Color c = Color(100, 100, 100);
#endif

    c = c.withMultipliedAlpha(min(t * 2, 1.f));
    float end = constrain((1 - t) * 2, 0, 1);

    if (strip->invertStrip)
        fillRange(c, 1 - end, 1);
    else
        fillRange(c, 0, end);
}