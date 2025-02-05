// Stream receiver
#pragma once




DeclareComponentSingleton(LedStreamReceiver, "streamReceiver", ESPNowDerive)

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
static void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data);
#endif

std::vector<LedStreamListener *> streamListeners;
void registerStreamListener(LedStreamListener *listener);
void unregisterStreamListener(LedStreamListener *listener);
void dispatchStreamData(uint16_t universe, const uint8_t *data, uint16_t len);

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
void onStreamReceived(const uint8_t *data, int len) override;
#endif



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