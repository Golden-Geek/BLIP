#include "UnityIncludes.h"

void LedStripStreamLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);

    AddIntParamConfig(universe);
    AddIntParamConfig(startChannel);
    AddBoolParamConfig(use16Bits);
    AddBoolParamConfig(includeAlpha);
    AddBoolParamConfig(clearOnNoReception);
    AddFloatParamConfig(noReceptionTime);
}

bool LedStripStreamLayer::initInternal()
{
    LedStreamReceiverComponent::instance->registerStreamListener(this);

    return true;
}

void LedStripStreamLayer::updateInternal()
{
    if (!hasCleared && clearOnNoReception && millis() / 1000.0f - lastReceiveTime > noReceptionTime)
    {
        clearColors();
        hasCleared = true;
    }
}

void LedStripStreamLayer::clearInternal()
{
    if (LedStreamReceiverComponent::instance != nullptr)
    {
        LedStreamReceiverComponent::instance->unregisterStreamListener(this);
    }
}

void LedStripStreamLayer::onLedStreamReceived(uint16_t dmxUniverse, const uint8_t *data, uint16_t len)
{
    int numChannels = includeAlpha ? 4 : 3;
    if (use16Bits)
        numChannels *= 2;

    const int maxCount = floor(512 / numChannels);
    int count = floor(len / numChannels);

    float multiplier = 1.0f;
    if (RootComponent::instance->isShuttingDown())
    {
        float relT = (millis() - RootComponent::instance->timeAtShutdown) / 1000.0f;
        const float animTime = 1.0f;
        multiplier = max(1 - relT * 2 / animTime, 0.f);
    }

    int numColors = strip->numColors;

    int numUniverses = std::ceil(numColors * 1.0f / maxCount); // maxCount leds per universe
    if (universe < universe || universe > universe + numUniverses - 1)
        return;

    // DBG("Received Artnet " + String(universe) + " " + String(len) +String(stripIndex) + " " + String(strip->numColors));

    int start = (dmxUniverse - universe) * maxCount - (startChannel - 1);

    int iStart = start < 0 ? -start : 0;

    // DBG("Received Artnet " + String(universe) + ", start = " + String(start));

    if (use16Bits)
    {
        for (int i = iStart; i < numColors && i < maxCount; i++)
        {
            float iMultiplier = multiplier * (includeAlpha ? (data[i * numChannels + 6] << 8 | data[i * numChannels + 7]) : 1.0f);
            const uint16_t r = (data[i * numChannels] << 8 | data[i * numChannels + 1]) * iMultiplier;
            const uint16_t g = (data[i * numChannels + 2] << 8 | data[i * numChannels + 3]) * iMultiplier;
            const uint16_t b = (data[i * numChannels + 4] << 8 | data[i * numChannels + 5]) * iMultiplier;
            colors[i + start] = Color(r, g, b);
        }
    }
    else
    {
        for (int i = iStart; i < numColors && i < maxCount; i++)
        {
            float iMultiplier = multiplier * (includeAlpha ? data[i * numChannels + 3] : 1.0f);
            colors[i + start] = Color(data[i * numChannels] * multiplier, data[i * numChannels + 1] * multiplier, data[i * numChannels + 2] * iMultiplier);
        }
    }

    lastReceiveTime = millis() / 1000.0f;
    hasCleared = false;
    // memcpy((uint8_t *)colors, streamBuffer + 1, byteIndex - 2);
}
