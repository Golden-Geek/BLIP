#pragma once

#ifdef USE_ARTNET
#define LEDSTREAM_MAX_LEDS 1000
#define LEDSTREAM_MAX_PACKET_SIZE LEDSTREAM_MAX_LEDS * 4 + 1
#define LEDSTREAM_ARTNET_PORT 5678
#endif

class LedStripStreamLayer : public LedStripLayer, public LedStreamListener
{
public:
    LedStripStreamLayer(LedStripComponent *strip) : LedStripLayer("streamLayer", LedStripLayer::Stream, strip) {}
    ~LedStripStreamLayer() {}

    DeclareIntParam(universe, 0);
    DeclareIntParam(startChannel, 1);
    DeclareBoolParam(includeAlpha, false);
    DeclareBoolParam(clearOnNoReception, true);
    DeclareFloatParam(noReceptionTime, 1.0f);

    bool hasCleared = false;
    float lastReceiveTime = 0;

    void setupInternal(JsonObject o) override;
    bool initInternal() override;
    void updateInternal() override;
    void clearInternal() override;

    void onLedStreamReceived(uint16_t universe, const uint8_t* data, uint16_t len) override;

    HandleSetParamInternalStart
        HandleSetParamInternalMotherClass(LedStripLayer)
            CheckAndSetParam(universe);
            CheckAndSetParam(startChannel);
    CheckAndSetParam(includeAlpha);
    CheckAndSetParam(clearOnNoReception);
    CheckAndSetParam(noReceptionTime);
    HandleSetParamInternalEnd;

    FillSettingsInternalStart
        FillSettingsInternalMotherClass(LedStripLayer)
            FillSettingsParam(universe);
            FillSettingsParam(startChannel);
    FillSettingsParam(includeAlpha);
    FillSettingsParam(clearOnNoReception);
    FillSettingsParam(noReceptionTime);
    FillSettingsInternalEnd;

    FillOSCQueryInternalStart
        FillOSCQueryInternalMotherClass(LedStripLayer)
            FillOSCQueryIntParam(universe);
            FillOSCQueryIntParam(startChannel);
    FillOSCQueryBoolParam(includeAlpha);
    FillOSCQueryBoolParam(clearOnNoReception);
    FillOSCQueryFloatParam(noReceptionTime);
    FillOSCQueryInternalEnd;
};
