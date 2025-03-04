#include "UnityIncludes.h"

void LedStripStreamLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);

    AddIntParamConfig(universe);
    AddIntParamConfig(startChannel);
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
    const int numChannels = includeAlpha ? 4 : 3;
    const int maxCount = includeAlpha ? 128 : 170;
    int count = len / numChannels;

    float multiplier = 1.0f;
    if (RootComponent::instance->isShuttingDown())
    {
        float relT = (millis() - RootComponent::instance->timeAtShutdown) / 1000.0f;
        const float animTime = 1.0f;
        multiplier = max(1 - relT * 2 / animTime, 0.f);
    }

    int numUniverses = std::ceil(strip->count * 1.0f / maxCount); // maxCount leds per universe
    if (universe < universe || universe > universe + numUniverses - 1)
        return;

    // DBG("Received Artnet " + String(universe) + " " + String(length) + " " + String(sequence) + " " + String(stripIndex) + " " + String(strip->count));

    int start = (dmxUniverse - universe) * maxCount - (startChannel - 1);

    int iStart = start < 0 ? -start : 0;

    // DBG("Received Artnet " + String(universe) + ", start = " + String(start));
    for (int i = iStart; i < strip->count && i < maxCount && (i * numChannels + 2) < len; i++)
    {
        colors[i + start] = Color(data[i * numChannels] * multiplier, data[i * numChannels + 1] * multiplier, data[i * numChannels + 2] * multiplier);
    }

    lastReceiveTime = millis() / 1000.0f;
    hasCleared = false;
    // memcpy((uint8_t *)colors, streamBuffer + 1, byteIndex - 2);
}
