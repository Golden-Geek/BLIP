// Stream receiver
#pragma once


DeclareComponentSingleton(DMXReceiver, "dmxReceiver", ESPNowDerive)

    DeclareIntParam(receiveRate, 60);

#ifdef USE_ARTNET
bool artnetIsInit;
float lastReceiveTime = 0;
ArtnetWifi artnet;
#endif

#ifdef USE_DMX
uint8_t data[512];
const dmx_port_t dmxPort = 1; // Use UART1
bool dmxIsInit;
#endif

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

#ifdef USE_ARTNET
void receiveArtnet();
#endif

#ifdef USE_DMX
void receiveDMX();
#endif

void onEnabledChanged() override;

void setupConnection();

#if defined USE_DMX || defined USE_ARTNET
static void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data);
#endif

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
void onDMXReceived(const uint8_t *data, int len) override;
#endif

std::vector<DMXListener *> dmxListeners;
void registerDMXListener(DMXListener *listener);
void unregisterDMXListener(DMXListener *listener);
void dispatchDMXData(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len);

HandleSetParamInternalStart
#if defined USE_DMX || defined USE_ARTNET
    CheckAndSetParam(receiveRate);
#endif
HandleSetParamInternalEnd;

FillSettingsInternalStart
#if defined USE_DMX || defined USE_ARTNET
    FillSettingsParam(receiveRate);
#endif
FillSettingsInternalEnd;

FillOSCQueryInternalStart
#if defined USE_DMX || defined USE_ARTNET
    FillOSCQueryIntParam(receiveRate);
#endif
FillOSCQueryInternalEnd;

EndDeclareComponent