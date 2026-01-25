#pragma once

#define ESPNOW_MAX_DEVICES 10
#define ESPNOW_MAX_STREAM_RECEIVERS 10 // How many components can listen here

#ifndef ESPNOW_PAIRING_PRESSCOUNT
#define ESPNOW_PAIRING_PRESSCOUNT -1
#endif

#ifndef ESPNOW_ENABLE_PRESSCOUNT
#define ESPNOW_ENABLE_PRESSCOUNT -1
#endif

#ifndef ESPNOW_DEFAULT_ENABLED
#define ESPNOW_DEFAULT_ENABLED false
#endif

#ifdef ESPNOW_BRIDGE
#define BridgeDMXDerive DMXListenerDerive
#else
#define BridgeDMXDerive
#endif

DeclareComponentSingletonEnabled(ESPNow, "espnow", BridgeDMXDerive, ESPNOW_DEFAULT_ENABLED)
    DeclareBoolParam(pairingMode, false);
DeclareBoolParam(longRange, false);
DeclareBoolParam(optimalRange, false);

DeclareIntParam(channel, 1);

#ifdef ESPNOW_BRIDGE
DeclareBoolParam(broadcastMode, true);
DeclareIntParam(broadcastStartID, 0);
DeclareIntParam(broadcastEndID, 255);
DeclareIntParam(streamUniverse, 0);
DeclareIntParam(streamStartChannel, 1);
DeclareBoolParam(streamTestMode, false);
DeclareIntParam(testLedCount, 50);
DeclareIntParam(streamTestRate, 50);
DeclareBoolParam(wakeUpMode, false);
DeclareBoolParam(routeAll, false);
DeclareBoolParam(acceptCommands, false);
std::string remoteMacs[ESPNOW_MAX_DEVICES];
uint8_t remoteMacsBytes[ESPNOW_MAX_DEVICES][6];
uint8_t numConnectedDevices = 0;

#else

DeclareBoolParam(autoPairing, true);
DeclareBoolParam(pairOnAnyData, true);
DeclareBoolParam(sendFeedback, false);

bool wakeUpReceived = false;

bool bridgeInit = false;
uint8_t bridgeMac[6] = {0, 0, 0, 0, 0, 0};
#endif

DeclareIntParam(enableMultiPressCount, ESPNOW_ENABLE_PRESSCOUNT);
DeclareIntParam(pairingMultiPressCount, ESPNOW_PAIRING_PRESSCOUNT);

#ifdef ESPNOW_BRIDGE
const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#endif

Color3 colorBuffer[1000]; // For streaming and test patterns

bool espNowInitialized = false;
long lastReceiveTime;
long lastSendTime = 0;
long lastCheck = 0;
uint8_t sendPacketData[250];
uint8_t sendMac[6];

ESPNowStreamReceiver *streamReceivers[ESPNOW_MAX_STREAM_RECEIVERS];

void setupInternal(JsonObject o) override;
bool initInternal() override;
void initESPNow();
void updateInternal() override;
void clearInternal() override;

void sendMessage(int id, const std::string &address, const std::string &command, var *data, int numData);

#ifdef ESPNOW_BRIDGE
void routeMessage(var *data, int numData);
void sendStream(int id, int universe, Color3 *colors, int numColors);
#endif

void sendPacket(int id, const uint8_t *data, int len);

void registerStreamReceiver(ESPNowStreamReceiver *receiver);
void unregisterStreamReceiver(ESPNowStreamReceiver *receiver);

static void onDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status);

static void onDataReceived(const esp_now_recv_info_t *mac, const uint8_t *incomingData, int len);
void dataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

void processMessage(const uint8_t *incomingData, int len);

#ifdef ESPNOW_BRIDGE

#if defined USE_DMX || defined USE_ARTNET
void onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) override;
#endif

void setupBroadcast();
void sendPairingRequest();
void sendPing();
void sendWakeUp();
void registerDevice(const uint8_t *deviceMac);
void addDevicePeer(const uint8_t *deviceMac);
void clearDevices();
#else
void registerBridgeMac(const uint8_t *_bridgeMac, int chan, bool sendPairing = true);
void sendPairingResponse(const uint8_t *bridgeMac);
#endif

void setupLongRange(const uint8_t *deviceMac);

void paramValueChangedInternal(ParamInfo *param) override;
bool handleCommandInternal(const std::string &command, var *data, int numData) override;

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

EndDeclareComponent;