#include "UnityIncludes.h"
#include "WifiComponent.h"

ImplementSingleton(WifiComponent)

    void WifiComponent::setupInternal(JsonObject o)
{
    state = Off;

    // AddAndSetParameter(ssid);
    // AddAndSetParameter(pass);
    // AddAndSetParameter(apOnNoWifi);

#ifdef USE_ETHERNET
    AddIntParamConfig(mode);
    WiFi.onEvent(std::bind(&WifiComponent::WiFiEvent, this, std::placeholders::_1));
#endif

    AddStringParamConfig(ssid);
    AddStringParamConfig(pass);
    AddStringParamConfig(manualIP);
    AddStringParamConfig(manualGateway);
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
#ifdef USE_ETHERNET
    if (mode == MODE_ETH)
        return;
#endif

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
                NDBG("Connection Error");
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
#ifdef USE_ETHERNET
        NDBG("Connected to ethernet with IP " + getIP());

#else
        NDBG("Connected to wifi " + ssid + " : " + pass + " with IP " + getIP() + " on channel " + String(WiFi.channel()));
#endif
        break;

    case ConnectionError:
        NDBG("Connection error");
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

        NDBG("ESPNow is running in normal mode, not connecting to wifi");
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

    WiFi.mode(wMode);

#ifdef ESP32
#endif

    setState(Connecting);
    waitingForIP = false;

#ifdef USE_ETHERNET
    if (mode == MODE_ETH || mode == MODE_ETH_STA)
    {

        if (manualIP != "" && manualGateway != "")
        {
            IPAddress ip, gateway, subnet(255, 255, 255, 0);

            String manualIPDots = manualIP;
            std::replace(manualIPDots.begin(), manualIPDots.end(), '_', '.');
            ip.fromString(manualIPDots);

            String manualGatewayDots = manualGateway;
            std::replace(manualGatewayDots.begin(), manualGatewayDots.end(), '_', '.');
            gateway.fromString(manualGatewayDots);

            ip.fromString(manualIPDots);
            gateway.fromString(manualGatewayDots);
            NDBG("Using manual IP configuration : " + ip.toString() + " with gateway " + gateway.toString());
            ETH.config(ip, gateway, subnet);
        }
        else
        {
            // Use DHCP
            NDBG("Using DHCP for IP configuration");
            ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        }

        ETH.begin();
    }
#endif

    if (wMode != WIFI_OFF)
    {
        NDBG("Connecting to wifi " + ssid + " : " + pass + "...");
        // WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        WiFi.setSleep(false);
        WiFi.setTxPower(WIFI_POWER_19dBm);
        if (manualIP != "" && manualGateway != "")
        {
            IPAddress ip, gateway, subnet(255, 255, 255, 0);

            String manualIPDots = manualIP;
            std::replace(manualIPDots.begin(), manualIPDots.end(), '_', '.');
            ip.fromString(manualIPDots);

            String manualGatewayDots = manualGateway;
            std::replace(manualGatewayDots.begin(), manualGatewayDots.end(), '_', '.');
            gateway.fromString(manualGatewayDots);

            ip.fromString(manualIPDots);
            gateway.fromString(manualGatewayDots);
            WiFi.config(ip, gateway, subnet);
            NDBG("Using manual IP configuration : " + ip.toString() + " with gateway " + gateway.toString());
        }
        else
        {
            // Use DHCP
            NDBG("Using DHCP for IP configuration");
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        }

        WiFi.begin(ssid.c_str(), pass.c_str());
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
        NDBG("Got IP !");
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

String WifiComponent::getIP() const
{
    if (state == Connected)
#ifdef USE_ETHERNET
        return StringHelpers::ipToString(ETH.localIP());
#else
        return StringHelpers::ipToString(WiFi.localIP());
#endif

    else if (state == Hotspot)
        return StringHelpers::ipToString(WiFi.softAPIP());

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