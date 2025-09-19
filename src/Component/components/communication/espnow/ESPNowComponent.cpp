#include "UnityIncludes.h"

ImplementSingleton(ESPNowComponent);

void ESPNowComponent::setupInternal(JsonObject o)
{
    AddBoolParamConfig(pairingMode);

#ifdef ESPNOW_BRIDGE
    AddIntParamConfig(channel);
    AddBoolParamConfig(broadcastMode);
    AddBoolParamConfig(streamTestMode);
    AddIntParamConfig(testLedCount);
    AddIntParamConfig(streamTestRate);
    AddBoolParam(wakeUpMode);
    AddBoolParamConfig(routeAll);

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

#else
    AddIntParamConfig(channel);
    AddBoolParamConfig(autoPairing);
    AddBoolParamConfig(pairOnAnyData);
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
    initESPNow();
#endif

    return true;
}

void ESPNowComponent::initESPNow()
{
    if (!enabled)
        return;

    if (!pairingMode)
    {
        NDBG("Init ESPNow on channel " + String(channel));
    }

#if defined USE_WIFI && defined ESPNOW_BRIDGE
#ifdef USE_ETHERNET
    NDBG("WiFi mode to STA");
    WiFi.mode(WIFI_AP_STA); // Recommended mode.
    NDBG("Setting channel " + String(channel));
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
#else
    SetParam(channel, WiFi.channel());
#endif
#else
    // esp_now_deinit();
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
#endif

    NDBG("Init ESPNow on channel " + String(channel));
    if (esp_now_init() != ESP_OK)
    {
        NDBG("Error initializing ESP-NOW");
        return;
    }

#ifdef ESPNOW_BRIDGE
    NDBG("ESP-NOW initialized as bridge");
    setupBroadcast();

#if USE_STREAMING
    LedStreamReceiverComponent::instance->registerStreamListener(this);
#endif
#endif

    esp_now_register_recv_cb(&ESPNowComponent::onDataReceived);

    if (!pairingMode)
    {
        NDBG("ESP-NOW accessible at  : " + SettingsComponent::instance->getDeviceID());
    }

    esp_now_register_send_cb(&ESPNowComponent::onDataSent);

#ifdef ESPNOW_BRIDGE
    NDBG("Registering devices...");
    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        if (remoteMacs[i]->length() > 0)
        {
            uint8_t mac[6];
            StringHelpers::macFromString(*(remoteMacs[i]), mac);
            DBG("Registering device " + StringHelpers::macToString(mac));
            addDevicePeer(mac);
        }
    }

    NDBG("Registered " + String(numConnectedDevices) + " devices");
#endif
}

void ESPNowComponent::updateInternal()
{
    unsigned long currentTime = millis();

#ifdef ESPNOW_BRIDGE
    if (pairingMode)
    {
        if (currentTime - lastSendTime >= 10)
        {
            lastSendTime = currentTime;
            sendPairingRequest();
        }
    }
    else if (wakeUpMode)
    {
        if (currentTime - lastSendTime >= 2)
        {
            lastSendTime = currentTime;
            sendWakeUp();
        }
    }
    else if (streamTestMode)
    {
        int streamMS = 1000 / max(1, streamTestRate);
        const int maxColorsPerPacket = 80; // 250 bytes, 3 bytes per color + some margin
        if (currentTime - lastSendTime >= streamMS)
        {
            lastSendTime = currentTime;

            for (int i = 0; i < testLedCount; i++)
            {
                testStreamColor[i] = Color::HSV(millis() / 2000.f + i * 1.f / testLedCount, 1, 1);


                if ((i + 1) % maxColorsPerPacket == 0)
                {
                    int universe = floor(i*1.f / maxColorsPerPacket);
                    // NDBG("Sending " + String(maxColorsPerPacket) + " leds to universe " + String(universe));
                    sendStream(-1, universe, testStreamColor + (i - maxColorsPerPacket), maxColorsPerPacket);
                }
            }

            int remaining = testLedCount % maxColorsPerPacket;
            if (remaining > 0)
            {
                int universe = floor(testLedCount*1.f / maxColorsPerPacket);
                // NDBG("Sending remaining " + String(remaining) + " leds to universe " + String(universe));
                sendStream(-1, universe, testStreamColor + (testLedCount - remaining), remaining);
            }
        }
    }
    else
    {
        // normal ping
        if (currentTime - lastSendTime >= 1000)
        {
            lastSendTime = currentTime;
            sendPing();
        }
    }
#else
    if (!pairingMode && currentTime - lastCheck > 500)
    {
        // NDBG("Checking ESPNow connection : " + String(currentTime - lastReceiveTime) + " ms since last receive, pairing Mode ? " + String(pairingMode));

        if (RootComponent::instance->remoteWakeUpMode || (autoPairing && currentTime - lastReceiveTime > 2000))
        {
            NDBG("Auto activate pairing mode");
            SetParam(pairingMode, true)
        }
        lastCheck = currentTime;
    }

    if (pairingMode)
    {
        if (currentTime - lastSendTime >= 100)
        {
            // DBG("Channel Hopping " + String(channel));
            lastSendTime = currentTime;
            channel = (channel + 1) % 16;
            initESPNow();
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
    {
        // DBG("[ESPNow] Sent to " + StringHelpers::macToString(mac_addr));
    }
    return;

    DBG("[ESPNow] Error sending to " + StringHelpers::macToString(mac_addr) + " : " + String(status));
}

#ifdef ARDUINO_NEW_VERSION
void ESPNowComponent::onDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
    const uint8_t *mac = info->src_addr;
#else
void ESPNowComponent::onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
#endif
    // DBG("[ESPNow] Data received from " + StringHelpers::macToString(mac) + " : " + String(len) + " bytes");

    instance->lastReceiveTime = millis();

#ifndef ESPNOW_BRIDGE
    if (instance->pairingMode && instance->pairOnAnyData)
    {
        instance->registerBridgeMac(mac, instance->channel, false);
    }
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

    case 2: // pairing request
    {
#ifndef ESPNOW_BRIDGE

        instance->registerBridgeMac(mac, incomingData[1]);

#endif
    }
    break;

    case 3: // pairing response
    {
#ifdef ESPNOW_BRIDGE
        // DBG("[ESPNow] Pairing response received");
        instance->registerDevice(mac);
#endif
    }
    break;

    case 4: // remote wakeup
    {
#ifndef ESPNOW_BRIDGE
        instance->wakeUpReceived = true;
#endif
    }
    break;

    default:
        break;
    }
}

void ESPNowComponent::sendMessage(int id, const String &address, const String &command, var *data, int numData)
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

    sendPacket(id, sendPacketData, dataIndex);
}

#ifdef ESPNOW_BRIDGE
void ESPNowComponent::routeMessage(var *data, int numData)
{
    String targetAddress = data[0].stringValue();

    bool shouldSend = false;
    int id = -1;
    if (routeAll)
        shouldSend = true;
    else
    {
        if (targetAddress.startsWith("dev."))
        {
            int idEnd = targetAddress.indexOf('.', 4);
            if (idEnd != -1)
            {
                id = targetAddress.substring(4, idEnd).toInt();
                targetAddress = targetAddress.substring(idEnd + 1);
                shouldSend = true;
            }
        }
    }

    sendMessage(id, targetAddress, data[1].stringValue(), &data[2], numData - 2);
}

void ESPNowComponent::sendStream(int id, int universe, Color *colors, int numColors)
{
    // Data format for stream
    //  0 - Stream (1)
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
    sendPacket(id, sendPacketData, totalLen);
}
#endif

void ESPNowComponent::sendPacket(int id, const uint8_t *data, int len)
{

#ifdef ESPNOW_BRIDGE
    // DBG("Sending packet to " + String(id) + " : " + String(len) + " bytes");
    if (id == -1)
    {
        if (broadcastMode)
        {
            // DBG("Broadcasting packet");
            esp_now_send(broadcastMac, data, len);
        }
        else
        {
            for (int i = 0; i < numConnectedDevices; i++)
            {
                // DBG("Sending to " + StringHelpers::macToString(remoteMacsBytes[i]));
                esp_now_send(remoteMacsBytes[i], data, len);
                if (i % 3 == 0)
                    delayMicroseconds(100);
            }
        }
    }
    else if (id >= 0 && id < numConnectedDevices)
    {
        // DBG("Sending to " + StringHelpers::macToString(remoteMacsBytes[id]));
        esp_now_send(remoteMacsBytes[id], data, len);
    }
#else
    // if (broadcastMode)
    // {
    //     esp_now_send(broadcastMac, data, len);
    // }
    // else
    // {
    esp_now_send(bridgeMac, data, len);
    // }
#endif
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
    const int cappedLen = min((int)len, 245); // max 245 bytes
    int totalLen = 5 + cappedLen;

    sendPacketData[0] = 1; // Stream
    sendPacketData[1] = (universe >> 24) & 0xFF;
    sendPacketData[2] = (universe >> 16) & 0xFF;
    sendPacketData[3] = (universe >> 8) & 0xFF;
    sendPacketData[4] = universe & 0xFF;

    memcpy(sendPacketData + 5, data, cappedLen);

    // wifi_set_channel(channel);
    // NDBG("Sending stream " + String(totalLen) + " bytes");
    // esp_now_send(NULL, sendPacketData, totalLen);
    sendPacket(-1, sendPacketData, totalLen);
}
#endif

#ifdef ESPNOW_BRIDGE

void ESPNowComponent::setupBroadcast()
{
    if (!isInit)
        return;

    if (broadcastMode || pairingMode || wakeUpMode)
    {
        esp_now_peer_info_t info;
        memset(&info, 0, sizeof(info));
        memcpy(info.peer_addr, broadcastMac, 6);

        info.channel = 0;
        info.encrypt = false;
        esp_now_add_peer(&info);
    }
    else
    {
        esp_now_del_peer(broadcastMac);
    }
}

void ESPNowComponent::sendPairingRequest()
{
    // DBG("Sending pairing request on channel " + String(WiFi.channel()));
    sendPacketData[0] = 2; // Pairing request
    sendPacketData[1] = WiFi.channel();
    esp_now_send(broadcastMac, sendPacketData, 2);
}

void ESPNowComponent::sendPing()
{
    // NDBG("Sending ping on channel " + String(WiFi.channel()));
    sendPacketData[0] = 2; // Pairing request
    sendPacketData[1] = WiFi.channel();
    sendPacket(-1, sendPacketData, 2);
}

void ESPNowComponent::sendWakeUp()
{
    // NDBG("Sending ping on channel " + String(WiFi.channel()));
    sendPacketData[0] = 4; // Pairing request
    sendPacketData[1] = WiFi.channel();
    esp_now_send(broadcastMac, sendPacketData, 2);
}

void ESPNowComponent::registerDevice(const uint8_t *deviceMac)
{
    String address = StringHelpers::macToString(deviceMac);

    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        if (*remoteMacs[i] == address)
        {
            // DBG("Device already registered : " + address);
            return;
        }
    }

    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
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
    memcpy(remoteMacsBytes[numConnectedDevices], deviceMac, 6);
    esp_err_t result = esp_now_add_peer(&info);

    switch (result)
    {
    case ESP_OK:
        NDBG("Device added : " + StringHelpers::macToString(deviceMac));
        break;
    case ESP_ERR_ESPNOW_NOT_INIT:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_NOT_INIT");
        break;
    case ESP_ERR_ESPNOW_ARG:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_ARG");
        break;
    case ESP_ERR_ESPNOW_FULL:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_FULL");
        break;
    case ESP_ERR_ESPNOW_NO_MEM:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_NO_MEM");
        break;
    case ESP_ERR_ESPNOW_EXIST:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_EXIST");
        break;
    case ESP_ERR_ESPNOW_IF:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_IF");
        break;
    case ESP_ERR_ESPNOW_NOT_FOUND:
        NDBG("Error adding devie : ESP_ERR_ESPNOW_NOT_FOUND");
        break;
    default:
        NDBG("Error adding devie : Unknown error");
        break;
    }
    numConnectedDevices++;

    String address = StringHelpers::macToString(deviceMac);
}

void ESPNowComponent::clearDevices()
{
    uint8_t *mac = (uint8_t *)malloc(6);

    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        if (*remoteMacs[i] == "")
            continue;

        DBG("Remove device " + *remoteMacs[i]);

        StringHelpers::macFromString(*remoteMacs[i], mac);

        esp_now_del_peer(mac);
        *remoteMacs[i] = "";
    }

    numConnectedDevices = 0;
    DBG("Devices cleared");
    SettingsComponent::instance->saveSettings();
}
#else

void ESPNowComponent::registerBridgeMac(const uint8_t *_bridgeMac, int chan, bool sendPairing)
{
    if (pairingMode)
    {
        SetParam(pairingMode, false);
        DBG("Found bridge on channel " + String(channel));
    }

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
    bridgeInit = true;

    SetParam(channel, chan);
    initESPNow();

    if (sendPairing)
        sendPairingResponse(bridgeMac);
}

void ESPNowComponent::sendPairingResponse(const uint8_t *_bridgeMac)
{
    // DBG("[ESPNow] Pairing request received, sending response to " + StringHelpers::macToString(_bridgeMac));
    sendPacketData[0] = 3; // Pairing response

    esp_now_send(bridgeMac, sendPacketData, 1);
}
#endif

void ESPNowComponent::paramValueChangedInternal(void *param)
{
    if (!isInit)
        return;

#ifdef ESPNOW_BRIDGE
    if (param == &pairingMode || param == &wakeUpMode)
    {
        if (param == &pairingMode)
            NDBG("Pairing mode " + String(pairingMode));
        else
            NDBG("Wake up mode " + String(wakeUpMode));

        setupBroadcast();

        if (param == &pairingMode && !pairingMode)
        {
            esp_now_peer_num_t peerCount;
            esp_now_get_peer_num(&peerCount);
            NDBG("Finished pairing, got " + String(numConnectedDevices) + " devices (peer count : " + String(peerCount.total_num) + ")");
            SettingsComponent::instance->saveSettings();
        }
    }

#else
    if (param == &pairingMode)
    {
        DBG("Pairing mode changed " + String(pairingMode));
        if (pairingMode)
            bridgeInit = false;
    }
#endif

#ifdef ESPNOW_BRIDGE
    if (param == &broadcastMode)
    {
        setupBroadcast();
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
