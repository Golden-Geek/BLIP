#pragma once


#define ESPNOW_MAX_DEVICES 20
#define ESPNOW_MAX_STREAM_RECEIVERS 10 //How many components can listen here

#ifndef ESPNOW_PAIRING_PRESSCOUNT
#define ESPNOW_PAIRING_PRESSCOUNT -1
#endif

#ifndef ESPNOW_ENABLE_PRESSCOUNT
#define ESPNOW_ENABLE_PRESSCOUNT -1
#endif

#ifndef ESPNOW_DEFAULT_ENABLED
#define ESPNOW_DEFAULT_ENABLED false
#endif

DeclareComponentSingletonEnabled(ESPNow, "espnow", LedStreamListenerDerive, ESPNOW_DEFAULT_ENABLED)
    DeclareBoolParam(pairingMode, false);

#ifdef USE_ETHERNET
DeclareIntParam(channel, 1);
#endif

#ifdef ESPNOW_BRIDGE
DeclareBoolParam(broadcastMode, false);
DeclareBoolParam(streamTestMode, false);
DeclareBoolParam(wakeUpMode, false);
DeclareBoolParam(routeAll, false);
DeclareStringParam(remoteMac1, "");
DeclareStringParam(remoteMac2, "");
DeclareStringParam(remoteMac3, "");
DeclareStringParam(remoteMac4, "");
DeclareStringParam(remoteMac5, "");
DeclareStringParam(remoteMac6, "");
DeclareStringParam(remoteMac7, "");
DeclareStringParam(remoteMac8, "");
DeclareStringParam(remoteMac9, "");
DeclareStringParam(remoteMac10, "");
DeclareStringParam(remoteMac11, "");
DeclareStringParam(remoteMac12, "");
DeclareStringParam(remoteMac13, "");
DeclareStringParam(remoteMac14, "");
DeclareStringParam(remoteMac15, "");
DeclareStringParam(remoteMac16, "");
DeclareStringParam(remoteMac17, "");
DeclareStringParam(remoteMac18, "");
DeclareStringParam(remoteMac19, "");
DeclareStringParam(remoteMac20, "");

String *remoteMacs[ESPNOW_MAX_DEVICES] = {
    &remoteMac1,
    &remoteMac2,
    &remoteMac3,
    &remoteMac4,
    &remoteMac5,
    &remoteMac6,
    &remoteMac7,
    &remoteMac8,
    &remoteMac9,
    &remoteMac10,
    &remoteMac11,
    &remoteMac12,
    &remoteMac13,
    &remoteMac14,
    &remoteMac15,
    &remoteMac16,
    &remoteMac17,
    &remoteMac18,
    &remoteMac19,
    &remoteMac20};

uint8_t remoteMacsBytes[ESPNOW_MAX_DEVICES][6] = {
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0}};

uint8_t numConnectedDevices = 0;

#else

DeclareIntParam(channel, 1);
DeclareBoolParam(autoPairing, true);

bool wakeUpReceived = false;

bool bridgeInit = false;
uint8_t bridgeMac[6] = {0, 0, 0, 0, 0, 0};
#endif

DeclareIntParam(enableMultiPressCount, ESPNOW_ENABLE_PRESSCOUNT);
DeclareIntParam(pairingMultiPressCount, ESPNOW_PAIRING_PRESSCOUNT);

#ifdef ESPNOW_BRIDGE
const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#endif

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

void sendMessage(int id, const String &address, const String &command, var *data, int numData);

#ifdef ESPNOW_BRIDGE
void routeMessage(var *data, int numData);
void sendStream(int id, int universe, Color *colors, int numColors);
#endif

void sendPacket(int id, const uint8_t *data, int len);

void registerStreamReceiver(ESPNowStreamReceiver *receiver);
void unregisterStreamReceiver(ESPNowStreamReceiver *receiver);

static void onDataSent(const uint8_t *mac, esp_now_send_status_t status);

#ifdef ARDUINO_NEW_VERSION
static void onDataReceived(const esp_now_recv_info_t *mac, const uint8_t *incomingData, int len);
#else
static void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);
#endif

#ifdef USE_STREAMING
void onLedStreamReceived(uint16_t universe, const uint8_t *data, uint16_t len) override;
#endif

#ifdef ESPNOW_BRIDGE
void setupBroadcast();
void sendPairingRequest();
void sendPing();
void sendWakeUp();
void registerDevice(const uint8_t *deviceMac);
void addDevicePeer(const uint8_t *deviceMac);
void clearDevices();
#else
void registerBridgeMac(const uint8_t *_bridgeMac, int chan);
void sendPairingResponse(const uint8_t *bridgeMac);
#endif

void paramValueChangedInternal(void *param) override;
bool handleCommandInternal(const String &command, var *data, int numData) override;

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

HandleSetParamInternalStart
    CheckAndSetParam(pairingMode);

#ifdef ESPNOW_BRIDGE
#ifdef USE_ETHERNET
CheckAndSetParam(channel);
#endif
CheckAndSetParam(broadcastMode);
CheckAndSetParam(streamTestMode);
CheckAndSetParam(wakeUpMode);
CheckTrigger(clearDevices);
CheckAndSetParam(routeAll);
CheckAndSetParam(remoteMac1);
CheckAndSetParam(remoteMac2);
CheckAndSetParam(remoteMac3);
CheckAndSetParam(remoteMac4);
CheckAndSetParam(remoteMac5);
CheckAndSetParam(remoteMac6);
CheckAndSetParam(remoteMac7);
CheckAndSetParam(remoteMac8);
CheckAndSetParam(remoteMac9);
CheckAndSetParam(remoteMac10);
CheckAndSetParam(remoteMac11);
CheckAndSetParam(remoteMac12);
CheckAndSetParam(remoteMac13);
CheckAndSetParam(remoteMac14);
CheckAndSetParam(remoteMac15);
CheckAndSetParam(remoteMac16);
CheckAndSetParam(remoteMac17);
CheckAndSetParam(remoteMac18);
CheckAndSetParam(remoteMac19);
CheckAndSetParam(remoteMac20);
#else
CheckAndSetParam(channel);
CheckAndSetParam(autoPairing);
#endif
HandleSetParamInternalEnd;

FillSettingsInternalStart
#ifdef ESPNOW_BRIDGE
#ifdef USE_ETHERNET
    FillSettingsParam(channel);
#endif
FillSettingsParam(broadcastMode);
FillSettingsParam(routeAll);
FillSettingsParam(remoteMac1);
FillSettingsParam(remoteMac2);
FillSettingsParam(remoteMac3);
FillSettingsParam(remoteMac4);
FillSettingsParam(remoteMac5);
FillSettingsParam(remoteMac6);
FillSettingsParam(remoteMac7);
FillSettingsParam(remoteMac8);
FillSettingsParam(remoteMac9);
FillSettingsParam(remoteMac10);
FillSettingsParam(remoteMac11);
FillSettingsParam(remoteMac12);
FillSettingsParam(remoteMac13);
FillSettingsParam(remoteMac14);
FillSettingsParam(remoteMac15);
FillSettingsParam(remoteMac16);
FillSettingsParam(remoteMac17);
FillSettingsParam(remoteMac18);
FillSettingsParam(remoteMac19);
FillSettingsParam(remoteMac20);
#else
    FillSettingsParam(channel);
FillSettingsParam(autoPairing);
#endif
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryBoolParam(pairingMode);
#ifdef ESPNOW_BRIDGE
FillOSCQueryBoolParam(wakeUpMode);
#ifdef USE_ETHERNET
FillOSCQueryIntParam(channel);
#else
FillOSCQueryIntParamReadOnly(channel);
#endif
FillOSCQueryBoolParam(broadcastMode);
FillOSCQueryBoolParam(streamTestMode);
FillOSCQueryTrigger(clearDevices);
FillOSCQueryBoolParam(routeAll);
#else
FillOSCQueryIntParam(channel);
FillOSCQueryBoolParam(autoPairing);
#endif

FillOSCQueryInternalEnd

    EndDeclareComponent;