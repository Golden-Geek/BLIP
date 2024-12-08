#pragma once

#ifdef USE_ARTNET
#define LEDSTREAM_MAX_LEDS 1000
#define LEDSTREAM_MAX_PACKET_SIZE LEDSTREAM_MAX_LEDS * 4 + 1
#define LEDSTREAM_ARTNET_PORT 5678
#endif



class LedStripStreamLayer : public LedStripLayer
{
public:
    LedStripStreamLayer(LedStripComponent *strip) : LedStripLayer("streamLayer", LedStripLayer::Stream, strip) {}
    ~LedStripStreamLayer() {}

    DeclareIntParam(universe, 0);
    DeclareBoolParam(clearOnNoReception, true);
    DeclareFloatParam(noReceptionTime, 1.0f);

    bool hasCleared = false;
    float lastReceiveTime = 0;

    void setupInternal(JsonObject o) override;
    bool initInternal() override;
    void updateInternal() override;
    void clearInternal() override;

    HandleSetParamInternalStart
        HandleSetParamInternalMotherClass(LedStripLayer)
            CheckAndSetParam(universe);
    CheckAndSetParam(clearOnNoReception);
    CheckAndSetParam(noReceptionTime);
    HandleSetParamInternalEnd;

    FillSettingsInternalStart
        FillSettingsInternalMotherClass(LedStripLayer)
            FillSettingsParam(universe);
    FillSettingsParam(clearOnNoReception);
    FillSettingsParam(noReceptionTime);
    FillSettingsInternalEnd;

    FillOSCQueryInternalStart
        FillOSCQueryInternalMotherClass(LedStripLayer)
            FillOSCQueryIntParam(universe);
    FillOSCQueryBoolParam(clearOnNoReception);
    FillOSCQueryFloatParam(noReceptionTime);
    FillOSCQueryInternalEnd;
};

// Stream receiver

#ifdef USE_ESPNOW
#define ESPNowDerive ,ESPNowStreamReceiver
#else
#define ESPNowDerive
#endif

DeclareComponentSingleton(LedStreamReceiver, "streamReceiver", ESPNowDerive  )

#ifdef USE_ARTNET
    DeclareIntParam(receiveRate, 60);
bool artnetIsInit;
float lastReceiveTime = 0;
ArtnetWifi artnet;
#endif

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

#ifdef USE_ARTNET
void receiveArtnet();
#endif

void onEnabledChanged() override;

void setupConnection();

#ifdef USE_ARTNET
uint8_t artnetBuffer[LEDSTREAM_MAX_PACKET_SIZE];
int byteIndex;
#endif



std::vector<LedStripStreamLayer *> layers;

void registerLayer(LedStripStreamLayer *layer);
void unregisterLayer(LedStripStreamLayer *layer);

#ifdef USE_ARTNET
static void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data);
#endif

#ifdef USE_ESPNOW
void onStreamReceived(const uint8_t *data, int len) override;
#endif

void handleReceiveData(uint16_t universe, uint16_t length, uint8_t *data);



HandleSetParamInternalStart
#ifdef USE_ARTNET
    CheckAndSetParam(receiveRate);
#endif
HandleSetParamInternalEnd;

FillSettingsInternalStart
#ifdef USE_ARTNET
    FillSettingsParam(receiveRate);
#endif
FillSettingsInternalEnd;

FillOSCQueryInternalStart
#ifdef USE_ARTNET
    FillOSCQueryIntParam(receiveRate);
#endif
FillOSCQueryInternalEnd;

EndDeclareComponent