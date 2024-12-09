// Stream receiver

#ifdef USE_ESPNOW
#ifdef ESPNOW_BRIDGE
#define ESPNowDerive

#else
#define ESPNowDerive , ESPNowStreamReceiver
#endif
#else
#define ESPNowDerive
#endif

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

#if USE_LEDSTRIP
std::vector<LedStripStreamLayer *> layers;
void registerLayer(LedStripStreamLayer *layer);
void unregisterLayer(LedStripStreamLayer *layer);
#endif

#ifdef USE_ARTNET
static void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data);
#endif

#ifdef USE_ESPNOW
#ifdef ESPNOW_BRIDGE
std::vector<LedStreamListener *> streamListeners;
void registerStreamListener(LedStreamListener *listener);
void unregisterStreamListener(LedStreamListener *listener);
#else
void onStreamReceived(const uint8_t *data, int len) override;
#endif
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