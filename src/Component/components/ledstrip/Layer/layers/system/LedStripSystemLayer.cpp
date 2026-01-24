#include "UnityIncludes.h"
#include "LedStripSystemLayer.h"

void LedStripSystemLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);
    blendMode = BlendMode::Alpha;

    AddBoolParam(showBattery);
}

void LedStripSystemLayer::updateInternal()
{

    clearColors();
    updateConnectionStatus();
    showBatteryStatus();
    updateShutdown();
}

void LedStripSystemLayer::clearInternal()
{
    clearColors();
}

void LedStripSystemLayer::showBatteryStatus()
{
#ifdef USE_BATTERY
    if (showBattery)
    {
        fillAll(Color(0, 0, 0)); // clear strip
        float val = BatteryComponent::instance->batteryLevel;
        fillRange(BatteryComponent::instance->getBatteryColor(), 0, val, false);
        return;
    }
#endif
}

void LedStripSystemLayer::updateConnectionStatus()
{

    if (WifiComponent::instance == NULL)
        return;

    int numColors = strip->numColors;

    bool showEspNow = false;

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
    showEspNow = ESPNowComponent::instance->enabled;
#endif

    if (showEspNow)
    {
#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE

        if (ESPNowComponent::instance->pairingMode || !ESPNowComponent::instance->bridgeInit)
        {
            // NDBG("Show espnow pairing mode " + String(ESPNowComponent::instance->pairingMode) + " bridge init " + String(ESPNowComponent::instance->bridgeInit));
            float t = millis() / 1000.0f;
            float val = (cos(t * 2 * PI / 5 + PI) * .5f + .5f) * .5f + .3f;

            fillAll(Color(0, 0, 0));
            if (ESPNowComponent::instance->pairingMode)
            {
                if (ESPNowComponent::instance->bridgeInit)
                    fillAll(Color(50, 255, 0).withMultipliedAlpha(val));
                else
                    fillAll(Color(espSyncColor[0], espSyncColor[1], espSyncColor[2], espSyncColor[3]).withMultipliedAlpha(val));
            }
            else
                fillAll(Color(250, 50, 0).withMultipliedAlpha(val));
        }

#endif
    }
    else
    {

        if (timeAtStartup == 0)
        {
            timeAtStartup = millis();
        }

        long time = millis() - timeAtStartup;

        float relT = (time - WifiComponent::instance->timeAtStateChange) / 1000.0f;
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
        float t = (time - RootComponent::instance->timeAtStart) / 1000.0f;
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
                targetColor = Color(30, 255, 0);
                break;
            case WifiComponent::ConnectionError:
                targetColor = Color(255, 30, 0);
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

            float blendFac = min(relT * 3, 1.f);
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
}

void LedStripSystemLayer::updateShutdown()
{
    if (!RootComponent::instance->isShuttingDown())
        return;

    float t = (millis() - RootComponent::instance->timeAtShutdown) / 1000.0f;
    float relT = t / SHUTDOWN_ANIM_TIME;

#ifdef USE_BATTERY
    if (BatteryComponent::instance->isBatteryLow())
    {
        // reverse saw red animation
        float val = max(1 - fmodf(relT * 3, 1) * 1.5f, 0.f);
        fillAll(Color(val * 255, 0, 0));
        return; // battery animation has priority
    }
#endif

    float alpha = min(relT * 3, 1.f);
    Color black = Color(0, 0, 0).withMultipliedAlpha(alpha);
    fillAll(black);

    Color c = Color(255, 20, 0);

    if (strip->numColors <= 2)
    {
        // from end 0-1 to 0-1-0 sine
        float val = sin(relT * PI);
        fillAll(c.withMultipliedAlpha(val));
    }
    else
    {
        float rel = constrain((1 - t) * 2, 0, 1);
        float start = strip->invertStrip ? 1 - rel : 0;
        float end = strip->invertStrip ? 1 : rel;
        fillRange(c.withMultipliedAlpha(alpha), start, end, false);
    }
}