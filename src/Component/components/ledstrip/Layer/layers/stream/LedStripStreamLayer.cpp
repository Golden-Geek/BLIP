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
            const uint16_t r = (data[channelIndex] << 8 | data[channelIndex + 1]);
            const uint16_t g = (data[channelIndex + 2] << 8 | data[channelIndex + 3]);
            const uint16_t b = (data[channelIndex + 4] << 8 | data[channelIndex + 5]);
            const uint16_t a = includeAlpha ? (data[channelIndex + 6] << 8 | data[channelIndex + 7]) : 16383;

            Color(r, g, b, a);
        }
    }
    else
    {
        for (int i = 0; i < numColors && i < maxLedCount && startingChannel + i * numChannels < len; i++)
        {
            int channelIndex = startingChannel + i * numChannels;
            colors[i] = Color(data[channelIndex],
                              data[channelIndex + 1],
                              data[channelIndex + 2],
                              includeAlpha ? data[channelIndex + 3] : 255);
        }
    }

    lastReceiveTime = millis() / 1000.0f;
    hasCleared = false;
}
