#pragma once

#define ESPNOW_MAX_STREAM_RECEIVERS 5

#ifdef USE_STREAMING
#define ListenerDerive ,LedStreamListener
#else
#define ListenerDerive 
#endif

DeclareComponentSingleton(ESPNow, "espnow", ListenerDerive )
    DeclareIntParam(channel, 1);

#ifdef ESPNOW_BRIDGE
DeclareStringParam(remoteMac, "30-AE-A4-F3-A3-88");
DeclareStringParam(remoteMac2, "30-AE-A4-31-2C-60");
#endif

long lastReceiveTime;
long lastSendTime = 0;
uint8_t sendPacketData[250];

esp_now_peer_info_t peerInfo;
esp_now_peer_info_t peerInfo2;
ESPNowStreamReceiver *streamReceivers[ESPNOW_MAX_STREAM_RECEIVERS];

void setupInternal(JsonObject o) override;
bool initInternal() override;
void initESPNow();
void updateInternal() override;
void clearInternal() override;

void sendMessage(const String& mac, const String &address, const String& command, var* data, int numData);
void sendStream(const String& mac, int universe, Color* colors, int numColors);

void registerStreamReceiver(ESPNowStreamReceiver *receiver);
void unregisterStreamReceiver(ESPNowStreamReceiver *receiver);

static void onDataSent(const uint8_t *mac, esp_now_send_status_t status);
static void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);

#ifdef USE_STREAMING
void onLedStreamReceived(uint16_t universe, const uint8_t* data, uint16_t len) override;
#endif

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

EndDeclareComponent;