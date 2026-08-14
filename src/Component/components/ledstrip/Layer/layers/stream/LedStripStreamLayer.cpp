#include "UnityIncludes.h"

void LedStripStreamLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);

    memset(pendingColors, 0, sizeof(pendingColors));

    AddIntParamConfig(universe);
    AddIntParamConfig(startChannel);
    AddBoolParamConfig(use16Bits);
    AddBoolParamConfig(includeAlpha);
    AddBoolParamConfig(clearOnNoReception);
    AddFloatParamConfig(noReceptionTime);
}

bool LedStripStreamLayer::initInternal()
{
    DMXReceiverComponent::instance->registerDMXListener(this);

    return true;
}

void LedStripStreamLayer::updateInternal()
{
    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    portENTER_CRITICAL(&pendingColorsMux);

    if (pendingFrame &&
        ((uint32_t)(nowUs - lastPacketTimeUs) >= LEDSTREAM_FRAME_SETTLE_US ||
         (uint32_t)(nowUs - pendingFrameStartUs) >= LEDSTREAM_MAX_FRAME_HOLD_US))
    {
        memcpy(colors, pendingColors, sizeof(Color) * strip->numColors);
        pendingFrame = false;
    }

    if (!hasCleared && clearOnNoReception &&
        (uint32_t)(nowMs - lastReceiveTimeMs) > (uint32_t)(noReceptionTime * 1000.0f))
    {
        memset(colors, 0, sizeof(Color) * strip->numColors);
        memset(pendingColors, 0, sizeof(Color) * strip->numColors);
        pendingFrame = false;
        hasCleared = true;
    }

    portEXIT_CRITICAL(&pendingColorsMux);
}

void LedStripStreamLayer::clearInternal()
{
    if (DMXReceiverComponent::instance != nullptr)
    {
        DMXReceiverComponent::instance->unregisterDMXListener(this);
    }
}

void LedStripStreamLayer::onDMXReceived(uint16_t dmxUniverse, const uint8_t *data, uint16_t startChannel, uint16_t len)
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

    if (actualLedCount <= 0 || dataStartIndex >= len)
        return;

    // DBG("Received Artnet, incoming universe : " + std::to_string(dmxUniverse) + ", strip universe : " + std::to_string(universe) + ", startChannel : " + std::to_string(startChannel) + ", ledStart : " + std::to_string(ledStart) + ", ledEnd : " + std::to_string(ledEnd) + ", actualLedCount : " + std::to_string(actualLedCount));

    const uint32_t packetTimeUs = micros();

    portENTER_CRITICAL(&pendingColorsMux);

    if (!pendingFrame)
    {
        // Preserve channels not carried by this packet while collecting the
        // rest of the frame.
        memcpy(pendingColors, colors, sizeof(Color) * numColors);
        pendingFrameStartUs = packetTimeUs;
        pendingFrame = true;
    }

    if (use16Bits)
    {
        for (int i = 0; i < actualLedCount; i++)
        {
            int channelIndex = dataStartIndex + i * colorDataSize;
            const uint16_t r = (data[channelIndex] << 8 | data[channelIndex + 1]);
            const uint16_t g = (data[channelIndex + 2] << 8 | data[channelIndex + 3]);
            const uint16_t b = (data[channelIndex + 4] << 8 | data[channelIndex + 5]);
            const uint16_t a = includeAlpha ? (data[channelIndex + 6] << 8 | data[channelIndex + 7]) : 16383;

            pendingColors[ledStart + i] = Color(r, g, b, a);
        }
    }
    else
    {
        for (int i = 0; i < actualLedCount; i++)
        {

            int channelIndex = dataStartIndex + i * colorDataSize;
            uint8_t alpha = includeAlpha ? data[channelIndex + 3] : 255;
            Color c = Color(data[channelIndex],
                            data[channelIndex + 1],
                            data[channelIndex + 2],
                            alpha);

            pendingColors[ledStart + i] = c;

            // if (i < 3)
            // {
            //     DBG("Data for LED " + std::to_string(ledStart + i) + ": R " + std::to_string(c.r) + " G " + std::to_string(c.g) + " B " + std::to_string(c.b) + " A " + std::to_string(c.a));
            // }
        }

    }

    lastPacketTimeUs = packetTimeUs;
    lastReceiveTimeMs = millis();
    hasCleared = false;

    portEXIT_CRITICAL(&pendingColorsMux);
}
