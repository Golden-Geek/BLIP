#pragma once

#ifdef ESP32

#ifdef USE_ETHERNET


#include <ETH.h>
#endif

#include <WiFi.h>

#ifdef USE_ESPNOW
#include <esp_wifi.h> // Include this library for esp_wifi_set_channel
#endif

#elif defined ESP8266
#include <ESP8266WiFi.h>
#endif

#ifndef WIFI_DEFAULT_MODE
#define WIFI_DEFAULT_MODE MODE_STA
#endif

DeclareComponentSingleton(Wifi, "wifi", )

    enum ConnectionState { Off,
                           Connecting,
                           Connected,
                           ConnectionError,
                           Disabled,
                           Hotspot,
                           PingAlive,
                           PingDead,
                           CONNECTION_STATES_MAX };

const String connectionStateNames[CONNECTION_STATES_MAX]{"Off", "Connecting", "Connected", "ConnectionError", "Disabled", "Hotspot", "PingAlive", "PingDead"};

const long timeBetweenTries = 500;    // ms
const long connectionTimeout = 20000; // ms
long timeAtConnect;
long lastConnectTime;
long timeAtStateChange;
long lastRSSIUpdate = 0;
bool waitingForIP = false;
ConnectionState state;

enum WifiMode
{
    MODE_STA,
    MODE_AP,
    MODE_AP_STA,
#ifdef USE_ETHERNET
    MODE_ETH,
    MODE_ETH_STA,
#endif
    MODE_MAX
};
const String wifiModeNames[MODE_MAX]{
    "Wifi",
    "AP",
    "Wifi+AP",
#ifdef USE_ETHERNET
    "Ethernet",
    "Wifi+Ethernet"
#endif
};

const wifi_power_t txPowerLevels[4] = {
    WIFI_POWER_15dBm,
    WIFI_POWER_17dBm,
    WIFI_POWER_19_5dBm,
    WIFI_POWER_20_5dBm};

const String txPowerLevelNames[4] = {
    "15dBm",
    "17dBm",
    "19.5dBm",
    "20.5dBm"};

DeclareStringParam(ssid, "");
DeclareStringParam(pass, "");
DeclareIntParam(channel, 0);
DeclareStringParam(manualIP, "");
DeclareStringParam(manualGateway, "");
DeclareBoolParam(channelScanMode, true);
DeclareIntParam(txPower, 2);

DeclareFloatParam(signal, 0)
    DeclareIntParam(mode, WIFI_DEFAULT_MODE);

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void connect();
void disconnect();
void setAP();

void disable();
void setState(ConnectionState s);

bool handleCommandInternal(const String &cmd, var *val, int numData) override;

#ifdef USE_ETHERNET
void WiFiEvent(WiFiEvent_t event);
#endif


bool isUsingEthernet() const;
bool isUsingWiFi() const;

String getIP() const;
int getChannel() const;

HandleSetParamInternalStart
    CheckAndSetParam(mode);
CheckAndSetParam(ssid);
CheckAndSetParam(pass);
CheckAndSetParam(channel);
CheckAndSetParam(manualIP);
CheckAndSetParam(manualGateway);
CheckAndSetParam(channelScanMode);
CheckAndSetParam(txPower);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(mode);
FillSettingsParam(ssid);
FillSettingsParam(pass);
FillSettingsParam(channel);
FillSettingsParam(manualIP);
FillSettingsParam(manualGateway);
FillSettingsParam(channelScanMode);
FillSettingsParam(txPower);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryEnumParam(mode, wifiModeNames, MODE_MAX);
FillOSCQueryStringParam(ssid);
FillOSCQueryStringParam(pass);
FillOSCQueryIntParam(channel);
FillOSCQueryFloatParamReadOnly(signal);
FillOSCQueryStringParam(manualIP);
FillOSCQueryStringParam(manualGateway);
FillOSCQueryBoolParam(channelScanMode);
FillOSCQueryEnumParam(txPower, txPowerLevelNames, 4);
FillOSCQueryInternalEnd;

DeclareComponentEventTypes(ConnectionStateChanged);
DeclareComponentEventNames("ConnectionStateChanged");

EndDeclareComponent