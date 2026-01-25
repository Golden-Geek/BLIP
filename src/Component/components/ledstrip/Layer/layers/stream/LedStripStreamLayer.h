#pragma once

#ifdef USE_ARTNET
#define LEDSTREAM_MAX_LEDS_PER_PACKET 170 // 170 leds if RGB, goes to 510 channels (not using the last 2 bytes of the 512 DMX packet)
#define LEDSTREAM_MAX_PACKET_SIZE LEDSTREAM_MAX_LEDS_PER_PACKET * 4 + 1
#define LEDSTREAM_ARTNET_PORT 5678
#endif

class LedStripStreamLayer : public LedStripLayer, public DMXListener
{
public:
    LedStripStreamLayer(LedStripComponent *strip) : LedStripLayer("streamLayer", LedStripLayer::Stream, strip) {}
    ~LedStripStreamLayer() {}

    DeclareIntParam(universe, 0);
    DeclareIntParam(startChannel, 1);
    DeclareBoolParam(use16Bits, false);
    DeclareBoolParam(includeAlpha, false);
    DeclareBoolParam(clearOnNoReception, true);
    DeclareFloatParam(noReceptionTime, 1.0f);

    bool hasCleared = false;
    float lastReceiveTime = 0;

    void setupInternal(JsonObject o) override;
    bool initInternal() override;
    void updateInternal() override;
    void clearInternal() override;

    void onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) override;
};
