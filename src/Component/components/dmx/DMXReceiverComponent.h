#pragma once
#if defined USE_DMX
#include "easydmx.h"
#endif

DeclareComponentSingleton(DMXReceiver, "dmxReceiver", ESPNowDerive)


#ifdef USE_ARTNET
bool artnetIsInit;
ArtnetWifi artnet;
#endif

#ifdef USE_DMX
DeclareIntParam(dmxActivityLedPin, DMX_ACTIVITY_LED_PIN);
EasyDMX dmx;
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

#if defined USE_ARTNET
static void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data);
#endif

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
void onStreamReceived(const uint8_t *data, int len) override;
#endif

std::vector<DMXListener *> dmxListeners;
void registerDMXListener(DMXListener *listener);
void unregisterDMXListener(DMXListener *listener);
void dispatchDMXData(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len);


EndDeclareComponent