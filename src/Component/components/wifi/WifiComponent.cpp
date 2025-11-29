#include "UnityIncludes.h"
#include "WifiComponent.h"

ImplementSingleton(WifiComponent)

    void WifiComponent::setupInternal(JsonObject o)
{
    state = Off;

    // AddAndSetParameter(ssid);
    // AddAndSetParameter(pass);
    // AddAndSetParameter(apOnNoWifi);
    AddIntParamConfig(mode);

#ifdef USE_ETHERNET
    WiFi.onEvent(std::bind(&WifiComponent::WiFiEvent, this, std::placeholders::_1));
#endif

    AddStringParamConfig(ssid);
    AddStringParamConfig(pass);
    AddStringParamConfig(manualIP);
    AddStringParamConfig(manualGateway);
    AddBoolParamConfig(channelScanMode);
    AddIntParamConfig(txPower);
    AddIntParamConfig(wifiProtocol);
    AddFloatParam(signal);

#ifdef WIFI_C6_USE_EXTERNAL_ANTENNA
    pinMode(3, OUTPUT);
    digitalWrite(3, LOW);
    delay(100);
    pinMode(14, OUTPUT);
    digitalWrite(14, HIGH);
#endif
}

bool WifiComponent::initInternal()
{
    connect();
    return true;
}

void WifiComponent::updateInternal()
{
    if (!isUsingWiFi())
        return;

    long curTime = millis();
    if (curTime > lastConnectTime + timeBetweenTries)
    {
        switch (state)
        {
        case Connecting:

#if defined ESP32
            if (WiFi.isConnected())
#elif defined ESP8266
            if (WiFi.status() == WL_CONNECTED)
#endif
            {
                // delay(100);
                if (WiFi.localIP() == IPAddress(0, 0, 0, 0))
                {
                    if (!waitingForIP)
                        NDBG("Waiting for IP...");
                    waitingForIP = true;
                    return;
                }

                setState(Connected);
                timeAtConnect = -1;
            }

            if (curTime > timeAtConnect + connectionTimeout)
            {
                // NDBG("Connection Error");
                setState(ConnectionError);
            }
            break;

        case Connected:
            if (!WiFi.isConnected())
            {
                NDBG("Lost connection ! will reconnect soon...");
                if (timeAtConnect == -1)
                {
                    timeAtConnect = millis();
                }
                else if (curTime > timeAtConnect + connectionTimeout)
                {
                    connect();
                }

                if (curTime > lastRSSIUpdate + 1000)
                {
                    lastRSSIUpdate = curTime;
                    float val = 1 / (1 + exp(-0.1 * (WiFi.RSSI() + 70)));
                    SetParam(signal, val);
                }
            }
            break;

        default:
            waitingForIP = false;
            break;
        }
    }
}

void WifiComponent::clearInternal()
{
    disconnect();
}

void WifiComponent::setState(ConnectionState s)
{
    if (state == s)
        return;

    state = s;
    switch (state)
    {
    case Connected:
    {
        String modeStr = isUsingEthernet() ? "Ethernet" : "Wifi";
        NDBG("Connected to " + modeStr + " " + ssid + " : " + pass + " with IP " + getIP() + " on channel " + String(WiFi.channel()));
    }
    break;

    case ConnectionError:
        NDBG("Connection Error");
        break;
    }

    timeAtStateChange = millis();
    sendEvent(ConnectionStateChanged);
}

void WifiComponent::connect()
{

#if defined USE_ESPNOW
    if (ESPNowComponent::instance->enabled)
    {
#ifndef ESPNOW_BRIDGE
        NDBG("ESPNow is running in normal mode, not connecting");
        return;
#endif
    }
#endif

    lastConnectTime = millis();
    timeAtConnect = millis();

    if (state == Connected || state == Hotspot)
        WiFi.disconnect();

    wifi_mode_t wMode = WIFI_STA;

    switch (mode)
    {
    case MODE_STA:
        wMode = WIFI_STA;
        break;
    case MODE_AP:
        wMode = WIFI_AP;
        break;
    case MODE_AP_STA:
        wMode = WIFI_AP_STA;
        break;

#ifdef USE_ETHERNET
    case MODE_ETH:
        wMode = WIFI_OFF;
        break;
    case MODE_ETH_STA:
        wMode = WIFI_STA;
        break;
#endif
    }

    NDBG("Setting WiFi mode to " + String(wMode));
    esp_wifi_set_protocol(WIFI_IF_STA, getWifiProtocol());
#ifdef ESP32
#endif

    setState(Connecting);
    waitingForIP = false;

#ifdef USE_ETHERNET
    if (isUsingEthernet())
    {
        NDBG("Starting Ethernet...");
        ETH.begin();
        
        if (manualIP != "" && manualGateway != "" && manualIP != "0.0.0.0" && manualGateway != "0.0.0.0")
        {
            IPAddress ip, gateway, subnet(255, 255, 255, 0);

            ip.fromString(manualIP);
            gateway.fromString(manualGateway);
            NDBG("Using manual IP configuration for Ethernet : " + ip.toString() + " with gateway " + gateway.toString());
            ETH.config(ip, gateway, subnet, IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1));
        }
        else
        {
            // Use DHCP
            NDBG("Using DHCP for IP configuration for Ethernet");
            // ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        }
    }
#endif

    if (isUsingWiFi())
    {
        // WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        WiFi.setSleep(false);

        if (channelScanMode)
        {
            WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
            WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
        }
        else
        {
            WiFi.setScanMethod(WIFI_FAST_SCAN);
        }

        WiFi.setTxPower((wifi_power_t)txPowerLevels[std::clamp(txPower, 0, MAX_POWER_LEVELS - 1)]);

        if (manualIP != "" && manualGateway != "" && manualIP != "0.0.0.0" && manualGateway != "0.0.0.0")
        {
            IPAddress ip, gateway, subnet(255, 255, 255, 0);
            ip.fromString(manualIP);
            gateway.fromString(manualGateway);
            WiFi.config(ip, gateway, subnet);
            NDBG("Using manual IP configuration for Wifi : " + ip.toString() + " with gateway " + gateway.toString());
        }
        else
        {
            // Use DHCP
            NDBG("Using DHCP for IP configuration for Wifi");
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        }

        NDBG("Connecting to wifi " + ssid + " : " + pass + "...");
        WiFi.begin(ssid.c_str(), pass.c_str(), channel);
    }
}

void WifiComponent::disable()
{
    WiFi.disconnect();
    setState(Disabled);
}

void WifiComponent::disconnect()
{
    WiFi.disconnect();
    setState(Off);
}

#ifdef USE_ETHERNET
void WifiComponent::WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {

    case ARDUINO_EVENT_ETH_START:
        ETH.setHostname(DeviceID.c_str());
        break;

    case ARDUINO_EVENT_ETH_CONNECTED:
        NDBG("Ethernet connected, getting IP...");
        // setState(Connected);
        break;

    case ARDUINO_EVENT_ETH_GOT_IP:
        setState(Connected);
        break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
        setState(ConnectionError);
        break;

    case ARDUINO_EVENT_ETH_STOP:
        setState(ConnectionError);
        break;

    default:
        break;
    }
}
#endif

bool WifiComponent::isUsingEthernet() const
{
#ifdef USE_ETHERNET
    return (mode == MODE_ETH || mode == MODE_ETH_STA);
#else
    return false;
#endif
}

bool WifiComponent::isUsingWiFi() const
{
#ifdef USE_ETHERNET
    if (mode == MODE_ETH_STA)
        return true;
#endif

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
    if (ESPNowComponent::instance->enabled)
    {
        return false;
    }
#endif

    return (mode == MODE_STA || mode == MODE_AP || mode == MODE_AP_STA);
}

String WifiComponent::getIP() const
{
    if (state == Connected)
    {
        bool showEth = false;
        String ethIP = "";
#ifdef USE_ETHERNET
        ethIP = ETH.localIP().toString();
#endif

        if (isUsingEthernet())
            return ethIP;
        else
            return WiFi.localIP().toString();
    }
    else if (state == Hotspot)
        return WiFi.softAPIP().toString();

    return "[noip]";
}
bool WifiComponent::handleCommandInternal(const String &cmd, var *val, int numData)
{
    if (cmd == "info")
    {
        String s = connectionStateNames[state] + " to " + ssid;
        s += "\nIP : " + getIP();
        s += "\nRSSI : " + String(WiFi.RSSI());
        NDBG(s);
        return true;
    }

    return false;
}

int WifiComponent::getChannel() const
{
    return WiFi.channel();
}

uint8_t WifiComponent::getWifiProtocol() const
{
    switch(wifiProtocol)
    {
        case WIFI_11B:
            return WIFI_PROTOCOL_11B;
        case WIFI_11BG:
            return WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G;
        case WIFI_11BGN:
            return WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;
        case WIFI_AX:
            return WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_11AX;
    }

    return WIFI_PROTOCOL_11B;
}