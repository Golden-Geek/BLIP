#pragma once

#ifdef ESP32

#ifdef USE_ETHERNET

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

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
#define WIFI_DEFAULT_MODE WIFI_MODE_STA
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
const long connectionTimeout = 10000; // ms
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
    "Wifi", "AP", "Wifi+AP",
#ifdef USE_ETHERNET
    "Ethernet", "Wifi+Ethernet"
#endif
};

DeclareStringParam(ssid, "");
DeclareStringParam(pass, "");
DeclareStringParam(manualIP, "");
DeclareStringParam(manualGateway, "");

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

String getIP() const;

HandleSetParamInternalStart
    CheckAndSetParam(ssid);
CheckAndSetParam(pass);
CheckAndSetParam(manualIP);
CheckAndSetParam(manualGateway);
#ifdef USE_ETHERNET
CheckAndSetParam(mode);
#endif
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(ssid);
FillSettingsParam(pass);
FillSettingsParam(manualIP);
FillSettingsParam(manualGateway);
#ifdef USE_ETHERNET
FillSettingsParam(mode);
#endif
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryStringParam(ssid);
FillOSCQueryStringParam(pass);
FillOSCQueryFloatParamReadOnly(signal);
FillOSCQueryStringParam(manualIP);
FillOSCQueryStringParam(manualGateway);
#ifdef USE_ETHERNET
FillOSCQueryEnumParam(mode, wifiModeNames, MODE_MAX);
#endif
FillOSCQueryInternalEnd;

DeclareComponentEventTypes(ConnectionStateChanged);
DeclareComponentEventNames("ConnectionStateChanged");

EndDeclareComponent