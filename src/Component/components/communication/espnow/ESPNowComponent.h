#pragma once

#define ESPNOW_MAX_STREAM_RECEIVERS 5

class ESPNowStreamReceiver
{
public:
  virtual void onStreamReceived(const uint8_t *data, int len) = 0;
};

DeclareComponentSingleton(ESPNow, "espnow", )
    DeclareIntParam(channel, 0);

#ifdef ESPNOW_BRIDGE
DeclareStringParam(remoteMac, "30-AE-A4-F3-A3-88");
#endif

bool hasReceivedData;
long lastSendTime;
uint8_t sendPacketData[250];

esp_now_peer_info_t peerInfo;
ESPNowStreamReceiver *streamReceivers[ESPNOW_MAX_STREAM_RECEIVERS];

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void sendMessage(const String& mac, const String &address, const String& command, var* data, int numData);
void sendStream(const String& mac, int universe, Color* colors, int numColors);

void registerStreamReceiver(ESPNowStreamReceiver *receiver);
void unregisterStreamReceiver(ESPNowStreamReceiver *receiver);

static void onDataSent(const uint8_t *mac, esp_now_send_status_t status);
static void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

EndDeclareComponent;