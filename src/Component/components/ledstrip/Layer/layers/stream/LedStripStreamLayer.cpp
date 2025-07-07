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

    const int maxLedCount = floor(512 / numChannels);
    int ledCount = floor(len / numChannels);

    float multiplier = 1.0f;
    if (RootComponent::instance->isShuttingDown())
    {
        float relT = (millis() - RootComponent::instance->timeAtShutdown) / 1000.0f;
        const float animTime = 1.0f;
        multiplier = max(1 - relT * 2 / animTime, 0.f);
    }

    int numColors = strip->numColors;

    int numUniverses = std::ceil(numColors * 1.0f / maxLedCount); // num universes needed to cover all colors
    if (dmxUniverse < universe || dmxUniverse >= universe + numUniverses)
    {
        return;
    }

    // DBG("Received Artnet, incoming universe : " + String(dmxUniverse) +", strip universe : "+String(universe) + ", strip num universes : " + String(numUniverses));

    
    int startingChannel = startChannel - 1; // startChannel is 1-based, convert to 0-based

    if (use16Bits)
    {
        for (int i = 0; i < numColors && i < maxLedCount && startingChannel + i * numChannels < len; i++)
        {
            int channelIndex = startingChannel + i * numChannels;
            float iMultiplier = multiplier * (includeAlpha ? (data[channelIndex + 6] << 8 | data[channelIndex + 7]) : 1.0f);
            const uint16_t r = (data[channelIndex] << 8 | data[channelIndex + 1]) * iMultiplier;
            const uint16_t g = (data[channelIndex + 2] << 8 | data[channelIndex + 3]) * iMultiplier;
            const uint16_t b = (data[channelIndex + 4] << 8 | data[channelIndex + 5]) * iMultiplier;
            colors[i] = Color(r, g, b);
        }
    }
    else
    {
        for (int i = 0; i < numColors && i < maxLedCount && startingChannel + i * numChannels < len; i++)
        {
            int channelIndex = startingChannel + i * numChannels;
            float iMultiplier = multiplier * (includeAlpha ? data[channelIndex + 3] : 1.0f);
            colors[i] = Color(data[channelIndex] * multiplier, 
                              data[channelIndex + 1] * multiplier, 
                              data[channelIndex + 2] * iMultiplier);
        }
    }

    lastReceiveTime = millis() / 1000.0f;
    hasCleared = false;
    // memcpy((uint8_t *)colors, streamBuffer + 1, byteIndex - 2);
}
