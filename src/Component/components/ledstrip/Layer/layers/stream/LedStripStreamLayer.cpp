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
    DMXReceiverComponent::instance->registerStreamListener(this);

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
    if (DMXReceiverComponent::instance != nullptr)
    {
        DMXReceiverComponent::instance->unregisterStreamListener(this);
    }
}

void LedStripStreamLayer::onLedStreamReceived(uint16_t dmxUniverse, const uint8_t *data, uint16_t startChannel, uint16_t len)
{
    int colorDataSize = includeAlpha ? 4 : 3;
    if (use16Bits)
        colorDataSize *= 2;

    const int maxLedCount = floor(512 / colorDataSize);
    int ledCount = floor(len / colorDataSize);

    int numColors = strip->numColors;

    int relUniverse = (dmxUniverse - universe);
    int relChannel = (startChannel - 1); // convert to 0-based

    int ledStart = relUniverse * maxLedCount + relChannel / colorDataSize;
    int ledEnd = ledStart + ledCount;

    int dataStartIndex = 0;
    if (ledStart < 0)
    {
        dataStartIndex = -ledStart * colorDataSize;
        ledStart = 0;
    }

    if (ledEnd > numColors)
        ledEnd = numColors;

    int actualLedCount = ledEnd - ledStart;

    // DBG("Received Artnet, incoming universe : " + String(dmxUniverse) + ", strip universe : " + String(universe) + ", startChannel : " + String(startChannel) + ", ledStart : " + String(ledStart) + ", ledEnd : " + String(ledEnd) + ", actualLedCount : " + String(actualLedCount));

    if (use16Bits)
    {
        for (int i = 0; i < actualLedCount; i++)
        {
            int channelIndex = dataStartIndex + i * colorDataSize;
            const uint16_t r = (data[channelIndex] << 8 | data[channelIndex + 1]);
            const uint16_t g = (data[channelIndex + 2] << 8 | data[channelIndex + 3]);
            const uint16_t b = (data[channelIndex + 4] << 8 | data[channelIndex + 5]);
            const uint16_t a = includeAlpha ? (data[channelIndex + 6] << 8 | data[channelIndex + 7]) : 16383;

            colors[ledStart + i] = Color(r, g, b, a);
        }
    }
    else
    {
        for (int i = 0; i < actualLedCount; i++)
        {

            int channelIndex = dataStartIndex + i * colorDataSize;
            Color c = Color(data[channelIndex],
                            data[channelIndex + 1],
                            data[channelIndex + 2],
                            includeAlpha ? data[channelIndex + 3] : 255);

            colors[ledStart + i] = c;

            // if (i < 3)
            // {
            //     DBG("Data for LED " + String(ledStart + i) + ": R " + String(c.r) + " G " + String(c.g) + " B " + String(c.b) + " A " + String(c.a));
            // }
        }

        lastReceiveTime = millis() / 1000.0f;
        hasCleared = false;
    }
}
