#include "UnityIncludes.h"
#include "ESPNowComponent.h"

ImplementSingleton(ESPNowComponent);

void ESPNowComponent::setupInternal(JsonObject o)
{
    AddIntParamConfig(channel);

#ifdef ESPNOW_BRIDGE
    AddBoolParamConfig(pairingMode);
    AddBoolParamConfig(streamTestMode);
    pairingMode = false; // force here

    AddStringParamConfig(remoteMac1);
    AddStringParamConfig(remoteMac2);
    AddStringParamConfig(remoteMac3);
    AddStringParamConfig(remoteMac4);
    AddStringParamConfig(remoteMac5);
    AddStringParamConfig(remoteMac6);
    AddStringParamConfig(remoteMac7);
    AddStringParamConfig(remoteMac8);
    AddStringParamConfig(remoteMac9);
    AddStringParamConfig(remoteMac10);
    AddStringParamConfig(remoteMac11);
    AddStringParamConfig(remoteMac12);
    AddStringParamConfig(remoteMac13);
    AddStringParamConfig(remoteMac14);
    AddStringParamConfig(remoteMac15);
    AddStringParamConfig(remoteMac16);
    AddStringParamConfig(remoteMac17);
    AddStringParamConfig(remoteMac18);
    AddStringParamConfig(remoteMac19);
    AddStringParamConfig(remoteMac20);

#endif

    lastReceiveTime = 0;

    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
        streamReceivers[i] = nullptr;
}

bool ESPNowComponent::initInternal()
{
#if defined USE_WIFI && defined ESPNOW_BRIDGE
    // wait for wifi connection
#else
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    initESPNow();
#endif

    return true;
}

void ESPNowComponent::initESPNow()
{
#if defined USE_WIFI && defined ESPNOW_BRIDGE
    channel = WiFi.channel();
#endif

    NDBG("Init ESPNow on channel " + String(channel));
    if (esp_now_init() != ESP_OK)
    {
        NDBG("Error initializing ESP-NOW");
        return;
    }

#ifdef ESPNOW_BRIDGE
    NDBG("ESP-NOW initialized as bridge");

#if USE_STREAMING
    LedStreamReceiverComponent::instance->registerStreamListener(this);
#endif
#endif

    esp_now_register_recv_cb(&ESPNowComponent::onDataReceived);
    NDBG("ESP-NOW accessible at  : " + SettingsComponent::instance->getDeviceID());

    esp_now_register_send_cb(&ESPNowComponent::onDataSent);

#ifdef ESPNOW_BRIDGE
    NDBG("Registering devices...");
    int numRegisteredDevices = 0;
    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        if (remoteMacs[i]->length() > 0)
        {
            uint8_t mac[6];
            StringHelpers::macFromString(*(remoteMacs[i]), mac);
            DBG("Registering device " + StringHelpers::macToString(mac));
            addDevicePeer(mac);
            numRegisteredDevices++;
        }
    }

    NDBG("Registered " + String(numRegisteredDevices) + " devices");
#endif
}

void ESPNowComponent::updateInternal()
{
#ifdef ESPNOW_BRIDGE
    unsigned long currentTime = millis();

    if (pairingMode)
    {
        if (currentTime - lastSendTime >= 500)
        {
            lastSendTime = currentTime;
            sendPairingRequest();
        }
    }
    else if (streamTestMode)
    {
        if (currentTime - lastSendTime >= 20)
        {
            lastSendTime = currentTime;

            Color colors[32];
            for (int i = 0; i < 16; i++)
            {
                colors[i] = Color::HSV(millis() / 2000.f + i / 32.f, 1, 1);
            }

            for (int i = 16; i < 32; i++)
            {
                colors[i] = Color(255, 0, 0);
            }

            sendStream("", 0, colors, 32);
        }
    }
#endif
}

void ESPNowComponent::clearInternal()
{
#if defined USE_STREAMING && defined ESPNOW_BRIDGE
    LedStreamReceiverComponent::instance->unregisterStreamListener(this);
#endif
}

void ESPNowComponent::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
        return;

    DBG("[ESPNow] Error sending to " + StringHelpers::macToString(mac_addr) + " : " + String(status));
}

void ESPNowComponent::onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    // DBG("[ESPNow] Data received from " + String(mac[0]) + ":" + String(mac[1]) + ":" + String(mac[2]) + ":" + String(mac[3]) + ":" + String(mac[4]) + ":" + String(mac[5]) + " : " + String(len) + " bytes");

    instance->lastReceiveTime = millis();

    // if (len < 2)
    // {
    //     DBG("[ESPNow] Not enough data received");
    //     return;
    // }

#ifndef ESPNOW_BRIDGE
    instance->registerBridgeMac(mac);
#endif

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

        String address = String((char *)(incomingData + 2), addressLength);

        uint8_t commandLength = incomingData[2 + addressLength];
        if (len < 3 + addressLength + commandLength)
        {
            DBG("[ESPNow] Command length more than data received");
            return;
        }

        String command = String((char *)(incomingData + 3 + addressLength), commandLength);

        var data[10];
        data[0] = address;
        data[1] = command;

        // DBG("[ESPNow] Message received from " + address + " : " + command + " : " + String(len) + " bytes");

        int dataIndex = 3 + addressLength + commandLength;

        int dataCount = 2;
        while (dataIndex < len)
        {
            if (dataCount >= 10)
            {
                DBG("[ESPNow] Too many data received");
                return;
            }

            data[dataCount].type = incomingData[dataIndex];
            dataIndex++;

            if (data[dataCount].type == 's')
            {
                uint8_t strLen = incomingData[dataIndex];
                dataIndex++;
                data[dataCount] = String((char *)(incomingData + dataIndex), strLen);
            }
            else if (data[dataCount].type == 'p')
            {
                uint8_t dataSize = incomingData[dataIndex];
                dataIndex++;
                data[dataCount] = var((uint8_t *)(incomingData + dataIndex), dataSize);
            }
            else if (data[dataCount].type == 'b')
            {
                data[dataCount].value.b = incomingData[dataIndex];
            }
            else if (data[dataCount].type == 'i')
            {
                memcpy(&data[dataCount].value.i, incomingData + dataIndex, sizeof(int));
            }
            else if (data[dataCount].type == 'f')
            {
                memcpy(&data[dataCount].value.f, incomingData + dataIndex, sizeof(float));
            }

            dataIndex += data[dataCount].getSize();
            dataCount++;
        }

        // DBG("[ESPNow] Message received from " + address + " : " + command + " : " + String(dataCount - 2));

        ESPNowComponent::instance->sendEvent(MessageReceived, data, dataCount);

#ifndef ESPNOW_BRIDGE
        // test send "ok" feedback
        // var feedbackData[1];
        // feedbackData[0] = "ok";
        // memcpy(instance->peerInfo.peer_addr, (void *)mac, 6);
        // esp_err_t t = esp_now_add_peer(&instance->peerInfo);
        // ESPNowComponent::instance->sendMessage("", "root", "log", feedbackData, 1);
#endif
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
    }
    break;

    case 2:
    {
#ifndef ESPNOW_BRIDGE
        instance->sendPairingResponse(mac);
#endif
    }
    break;

    case 3:
    {
#ifdef ESPNOW_BRIDGE
        // DBG("[ESPNow] Pairing response received");
        instance->registerDevice(mac);
#endif
    }
    break;

    default:
        break;
    }
}

void ESPNowComponent::sendMessage(const String &mac, const String &address, const String &command, var *data, int numData)
{
    // Data format for message
    //  0 - Message
    //  1 - Address length
    //  2 - Address
    //  3 - Command length
    //  4 - Command
    //  for each var
    //  5 - var type
    //  6 - var length (if string or ptr)
    //  7 - var data

    // NDBG("Sending message to " + mac + " : " + address + " : " + command);

    sendPacketData[0] = 0; // Message
    sendPacketData[1] = address.length();
    memcpy(sendPacketData + 2, address.c_str(), address.length());
    sendPacketData[2 + address.length()] = command.length();
    memcpy(sendPacketData + 3 + address.length(), command.c_str(), command.length());

    int dataIndex = 3 + address.length() + command.length();
    for (int i = 0; i < numData; i++)
    {
        sendPacketData[dataIndex] = data[i].type;
        dataIndex++;
        if (data[i].type == 'p')
        {
            sendPacketData[dataIndex++] = data[i].getSize();
            memcpy(sendPacketData + dataIndex, data[i].value.ptr, data[i].getSize());
        }
        else if (data[i].type == 's')
        {
            sendPacketData[dataIndex++] = data[i].getSize();
            memcpy(sendPacketData + dataIndex, data[i].stringValue().c_str(), data[i].getSize());
        }
        else if (data[i].type == 'b')
            sendPacketData[dataIndex] = data[i].boolValue();
        else if (data[i].type == 'i')
        {
            memcpy(sendPacketData + dataIndex, &data[i].value.i, sizeof(int));
        }
        else if (data[i].type == 'f')
        {
            memcpy(sendPacketData + dataIndex, &data[i].value.f, sizeof(float));
        }

        dataIndex += data[i].getSize();
    }

    if (dataIndex > 250)
    {
        DBG("Message data too long, max 250 bytes allowed.");
        return;
    }

    esp_now_send(NULL, sendPacketData, dataIndex);
}

void ESPNowComponent::sendStream(const String &mac, int universe, Color *colors, int numColors)
{
    // Data format for stream
    //  0 - Stream
    //  1-4 - Universe
    //  for each color
    //  R
    //  G
    //  B

    uint8_t totalLen = 5 + numColors * 3;
    if (totalLen > 250)
    {
        DBG("Stream data too long, max ~80 colors allowed per packet.");
        return;
    }

    sendPacketData[0] = 1; // Stream
    sendPacketData[1] = (universe >> 24) & 0xFF;
    sendPacketData[2] = (universe >> 16) & 0xFF;
    sendPacketData[3] = (universe >> 8) & 0xFF;
    sendPacketData[4] = universe & 0xFF;

    for (int i = 0; i < numColors; i++)
    {
        sendPacketData[5 + i * 3] = colors[i].r;
        sendPacketData[5 + i * 3 + 1] = colors[i].g;
        sendPacketData[5 + i * 3 + 2] = colors[i].b;
    }

    // wifi_set_channel(channel);
    esp_now_send(NULL, sendPacketData, totalLen);
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

#ifdef USE_STREAMING
void ESPNowComponent::onLedStreamReceived(uint16_t universe, const uint8_t *data, uint16_t len)
{
    const int cappedLen = min((int)len, 32 * 3); // max 80 leds
    int totalLen = 5 + cappedLen;

    sendPacketData[0] = 1; // Stream
    sendPacketData[1] = (universe >> 24) & 0xFF;
    sendPacketData[2] = (universe >> 16) & 0xFF;
    sendPacketData[3] = (universe >> 8) & 0xFF;
    sendPacketData[4] = universe & 0xFF;

    memcpy(sendPacketData + 5, data, cappedLen);

    // wifi_set_channel(channel);
    // NDBG("Sending stream " + String(totalLen) + " bytes");
    esp_now_send(NULL, sendPacketData, totalLen);
}
#endif

#ifdef ESPNOW_BRIDGE
void ESPNowComponent::sendPairingRequest()
{
    var data[1];
    data[0] = SettingsComponent::instance->getDeviceID();
    DBG("Sending pairing request");
    sendPacketData[0] = 2; // Pairing request
    esp_now_send(broadcastMac, sendPacketData, 1);
}

void ESPNowComponent::registerDevice(const uint8_t *deviceMac)
{
    String address = StringHelpers::macToString(deviceMac);

    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
    {
        if (*remoteMacs[i] == address)
        {
            DBG("Device already registered : " + address);
            return;
        }
    }

    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
    {
        if ((*remoteMacs[i]).isEmpty())
        {
            *remoteMacs[i] = address;

            addDevicePeer(deviceMac);
            return;
        }
    }

    DBG("No empty slot available to register device for " + address);
}

void ESPNowComponent::addDevicePeer(const uint8_t *deviceMac)
{
    esp_now_peer_info_t info;
    memset(&info, 0, sizeof(info));
    info.channel = 0;
    info.encrypt = false;
    memcpy(info.peer_addr, deviceMac, 6);
    esp_now_add_peer(&info);

    String address = StringHelpers::macToString(deviceMac);
    DBG(String("Device added: ") + address);
}

void ESPNowComponent::clearDevices()
{
    uint8_t *mac = (uint8_t *)malloc(6);

    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
    {
        if (*remoteMacs[i] == "")
            continue;

        DBG("Remove device " + *remoteMacs[i]);

        StringHelpers::macFromString(*remoteMacs[i], mac);

        esp_now_del_peer(mac);
        *remoteMacs[i] = "";
    }

    DBG("Devices cleared");
    SettingsComponent::instance->saveSettings();
}
#else

void ESPNowComponent::registerBridgeMac(const uint8_t *_bridgeMac)
{
    if (bridgeInit)
        return;

    // DBG("Registering bridge mac");
    // esp_now_del_peer(bridgeMac);

    memcpy(bridgeMac, _bridgeMac, 6);

    esp_now_peer_info_t info;
    memset(&info, 0, sizeof(info));
    info.channel = 0;
    info.encrypt = false;
    memcpy(info.peer_addr, bridgeMac, 6);
    esp_now_add_peer(&info);
}

void ESPNowComponent::sendPairingResponse(const uint8_t *_bridgeMac)
{
    DBG("[ESPNow] Pairing request received, sending response to " + StringHelpers::macToString(_bridgeMac));

    // DBG("Sending pairing response");
    sendPacketData[0] = 3; // Pairing response
    // for (int i = 0; i < 6; i++)
    //     sendPacketData[i + 1] = mac[i];

    esp_now_send(bridgeMac, sendPacketData, 1);
}
#endif

void ESPNowComponent::paramValueChangedInternal(void *param)
{
#ifdef ESPNOW_BRIDGE
    if (param == &pairingMode)
    {
        DBG("Pairing mode " + String(pairingMode));

        if (pairingMode)
        {
            esp_now_peer_info_t info;
            memset(&info, 0, sizeof(info));
            for (int i = 0; i < ESP_NOW_ETH_ALEN; i++)
                info.peer_addr[i] = 0xff; // add broadcast FF:FF:FF:FF:FF:FF

            info.channel = 0;
            info.encrypt = false;
            esp_now_add_peer(&info);
        }
        else
        {
            DBG("Finished pairing, saving settings");
            SettingsComponent::instance->saveSettings();
        }
    }
#endif
}

bool ESPNowComponent::handleCommandInternal(const String &command, var *data, int numData)
{
#ifdef ESPNOW_BRIDGE
    if (command == "clear")
    {
        clearDevices();
        return true;
    }
#endif

    return false;
}
