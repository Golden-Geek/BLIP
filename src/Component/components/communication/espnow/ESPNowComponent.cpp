#include "UnityIncludes.h"

ImplementSingleton(ESPNowComponent);

void ESPNowComponent::setupInternal(JsonObject o)
{
    peerInfo.channel = 0;
    AddIntParamConfig(channel);

#ifdef ESPNOW_BRIDGE
    AddStringParamConfig(remoteMac);
#endif

    hasReceivedData = false;
    lastSendTime = 0;

    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
        streamReceivers[i] = nullptr;
}

bool ESPNowComponent::initInternal()
{
#ifdef USE_WIFI
    if (WifiComponent::instance->state != WifiComponent::ConnectionState::Off && WifiComponent::instance->state != WifiComponent::ConnectionState::Hotspot)
    {
        NDBG("Wifi is connected, can't init ESP-NOW");
        return false;
    }

    WiFi.mode(WIFI_STA);
#endif

    if (esp_now_init() != ESP_OK)
    {
        NDBG("Error initializing ESP-NOW");
        return false;
    }

#ifdef ESPNOW_BRIDGE
    uint8_t remoteMacArr[6];
    sscanf(remoteMac.c_str(), "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx", 
           &remoteMacArr[0], &remoteMacArr[1], &remoteMacArr[2], 
           &remoteMacArr[3], &remoteMacArr[4], &remoteMacArr[5]);

    memcpy(peerInfo.peer_addr, remoteMacArr, 6);
    esp_now_add_peer(&peerInfo);

    NDBG("ESP-NOW initialized as bridge, added " + remoteMac);

#else
    esp_now_register_recv_cb(&ESPNowComponent::onDataReceived);
    NDBG("ESP-NOW accessible at  : " + SettingsComponent::instance->getDeviceID());
#endif

    esp_now_register_send_cb(&ESPNowComponent::onDataSent);
    return true;
}

void ESPNowComponent::updateInternal()
{
#ifdef ESPNOW_BRIDGE
    unsigned long currentTime = millis();

    if (currentTime - lastSendTime >= 1000)
    {
        lastSendTime = currentTime;
        sendMessage("", "root.stats", (uint8_t *)"ping", 4);
    }
#endif
}

void ESPNowComponent::clearInternal()
{
}

void ESPNowComponent::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    char macStr[18];
    String statusStr = String(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    DBG("[ESPNow] Send to: " + String(mac_addr[0]) + ":" + String(mac_addr[1]) + ":" + String(mac_addr[2]) + ":" + String(mac_addr[3]) + ":" + String(mac_addr[4]) + ":" + String(mac_addr[5]) + String(" > ") + statusStr);
}

void ESPNowComponent::onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    instance->hasReceivedData = true;

    if (len < 2)
    {
        DBG("[ESPNow] Not enough data received");
        return;
    }

    switch (incomingData[0])
    {
    case 0: // Message
    {
        uint8_t addressLength = incomingData[1];

        if (len < 2 + addressLength)
        {
            DBG("[ESPNow] Address length more than data received");
            return;
        }

        String address = String((char *)(incomingData + 2));

        int remainingData = len - 2 - addressLength;

        var data[2];
        data[0] = address;
        data[1] = var(incomingData + 2 + addressLength, remainingData);

        ESPNowComponent::instance->sendEvent(MessageReceived, data, 2);
    }
    break;

    case 1: // Stream
    {
        for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
        {
            if (ESPNowComponent::instance->streamReceivers[i] != nullptr)
            {
                ESPNowComponent::instance->streamReceivers[i]->onStreamReceived(incomingData + 1, len - 1);
            }
        }
        break;
    }
    }
}

void ESPNowComponent::sendMessage(const String &mac, const String &address, const uint8_t *data, int len)
{
    uint8_t addressLength = address.length();
    uint8_t totalLen = 2 + addressLength + len;
    uint8_t *fullData = (uint8_t *)malloc(totalLen);

    fullData[0] = 0; // Message
    fullData[1] = addressLength;
    memcpy(fullData + 2, address.c_str(), addressLength);
    memcpy(fullData + 2 + addressLength, data, len);

    // wifi_set_channel(channel);
    esp_now_send(NULL, fullData, totalLen);
    free(fullData);
}

void ESPNowComponent::sendStream(const String &mac, int universe, Color *colors, int numColors)
{
    uint8_t *fullData = (uint8_t *)malloc(1 + numColors * 3);
    fullData[0] = 1; // Stream

    for (int i = 0; i < numColors; i++)
    {
        fullData[1 + i * 3] = colors[i].r;
        fullData[1 + i * 3 + 1] = colors[i].g;
        fullData[1 + i * 3 + 2] = colors[i].b;
    }

    // wifi_set_channel(channel);
    esp_now_send(NULL, fullData, 1 + numColors * 3);
    free(fullData);
}

void ESPNowComponent::registerStreamReceiver(ESPNowStreamReceiver *receiver)
{
    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
    {
        if (ESPNowComponent::instance->streamReceivers[i] == nullptr)
        {
            ESPNowComponent::instance->streamReceivers[i] = receiver;
            return;
        }
    }

    DBG("No more stream receivers available");
}

void ESPNowComponent::unregisterStreamReceiver(ESPNowStreamReceiver *receiver)
{
    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
    {
        if (ESPNowComponent::instance->streamReceivers[i] == receiver)
        {
            ESPNowComponent::instance->streamReceivers[i] = nullptr;
            return;
        }
    }
}
