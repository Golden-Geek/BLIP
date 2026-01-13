#include "UnityIncludes.h"

ImplementSingleton(ESPNowComponent);

void ESPNowComponent::setupInternal(JsonObject o)
{
    AddBoolParamConfig(pairingMode);
    AddBoolParamConfig(longRange);
    AddBoolParamConfig(optimalRange);

    AddIntParamConfig(channel);

#ifdef ESPNOW_BRIDGE
    AddBoolParamConfig(broadcastMode);
    AddIntParamConfig(broadcastStartID);
    AddIntParamConfig(broadcastEndID);
    AddIntParamConfig(streamUniverse);
    AddIntParamConfig(streamStartChannel);
    AddBoolParamConfig(streamTestMode);
    AddIntParamConfig(testLedCount);
    AddIntParamConfig(streamTestRate);
    AddBoolParam(wakeUpMode);
    AddBoolParamConfig(routeAll);
    AddBoolParamConfig(acceptCommands);

    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        AddStringParamConfig(remoteMacs[i]);
    }

#else
    AddBoolParamConfig(autoPairing);
    AddBoolParamConfig(pairOnAnyData);
    AddBoolParamConfig(sendFeedback);
#endif

    lastReceiveTime = 0;

    for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
        streamReceivers[i] = nullptr;
}

bool ESPNowComponent::initInternal()
{
#if defined USE_WIFI && defined ESPNOW_BRIDGE
    // wait for wifi connection
    // if (!WifiComponent::instance->isUsingWiFi())
    // initESPNow();
#else
    initESPNow();
#endif

    return true;
}

void ESPNowComponent::initESPNow()
{
    espNowInitialized = false;

    if (!enabled)
        return;

#if defined USE_WIFI

    const bool wifiActive = WifiComponent::instance->isUsingWiFi();
    if (!wifiActive)
    {
        // Ensure the WiFi driver is at least started in STA mode before touching ESP-NOW.
        if (WiFi.getMode() != WIFI_STA)
        {
            WiFi.mode(WIFI_STA);
        }
        WiFi.setSleep(false);
    }

#if defined ESPNOW_BRIDGE

#ifdef USE_ETHERNET
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
#else
    SetParam(channel, WifiComponent::instance->getChannel());
#endif

#else
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
#endif // END ESPNOW_BRIDGE

    if (!wifiActive)
    {
        NDBG("WiFi mode to STA with Phy " + WifiComponent::instance->wifiProtocolNames[WifiComponent::instance->wifiProtocol]);
        esp_wifi_set_protocol(WIFI_IF_STA, WifiComponent::instance->getWifiProtocol());
        int powerIndex = std::clamp(WifiComponent::instance->txPower, 0, MAX_POWER_LEVELS - 1);
        NDBG("Setting TX Power to " + WifiComponent::instance->txPowerLevelNames[powerIndex]);
        WiFi.setTxPower((wifi_power_t)WifiComponent::instance->txPowerLevels[powerIndex]);
    }
    else
    {
        NDBG("WiFi already initialized");
    }
#endif // END USE_WIFI

    NDBG("Init ESPNow on channel " + String(channel));
    if (esp_now_init() != ESP_OK)
    {
        NDBG("Error initializing ESP-NOW");
        return;
    }

#ifdef ESPNOW_BRIDGE
    NDBG("ESP-NOW initialized as bridge");
    setupBroadcast();

#if defined USE_DMX || defined USE_ARTNET
    DMXReceiverComponent::instance->registerDMXListener(this);
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
        if (remoteMacs[i].length() > 0)
        {
            uint8_t mac[6];
            StringHelpers::macFromString(remoteMacs[i], mac);
            DBG("Registering device " + StringHelpers::macToString(mac));
            addDevicePeer(mac);
        }
    }

    NDBG("Registered " + String(numConnectedDevices) + " devices");

#endif

    espNowInitialized = true;
}

void ESPNowComponent::updateInternal()
{
    if (!espNowInitialized)
        return;

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
        if (currentTime - lastSendTime >= streamMS)
        {
            lastSendTime = currentTime;

            for (int i = 0; i < testLedCount; i++)
            {
                Color c = Color::HSV(millis() / 2000.f + i * 1.f / testLedCount, 1, 1);
                colorBuffer[i] = Color3(c.r, c.g, c.b);
            }

            sendStream(-1, 0, colorBuffer, testLedCount);
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
#if (defined USE_DMX || defined USE_ARTNET) && defined ESPNOW_BRIDGE
    DMXReceiverComponent::instance->unregisterDMXListener(this);
#endif
}

void ESPNowComponent::onDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        // DBG("[ESPNow] Sent to " + StringHelpers::macToString(tx_info->des_addr));
    }
    return;

    DBG("[ESPNow] Error sending to " + StringHelpers::macToString(tx_info->des_addr) + " : " + String(status));
}

void ESPNowComponent::onDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
    instance->dataReceived(info, incomingData, len);
}

void ESPNowComponent::dataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
    const uint8_t *mac = info->src_addr;
    // DBG("[ESPNow] Data received from " + StringHelpers::macToString(mac) + " : " + String(len) + " bytes");

    lastReceiveTime = millis();

    RootComponent::instance->timeAtLastSignal = millis();

#ifndef ESPNOW_BRIDGE
    if ((pairingMode || !bridgeInit) && pairOnAnyData)
    {
        registerBridgeMac(mac, channel, false);
    }
#endif

    switch (incomingData[0])
    {
    case 0: // Message
    {
        processMessage(incomingData, len);
    }
    break;

    case 1: // Stream
    {
        // DBG("[ESPNow] Stream data received: " + String(len - 1) + " bytes");
        for (int i = 0; i < ESPNOW_MAX_STREAM_RECEIVERS; i++)
        {
            if (ESPNowComponent::streamReceivers[i] != nullptr)
            {
                ESPNowComponent::streamReceivers[i]->onDMXReceived(incomingData + 1, len - 1);
            }
        }
    }
    break;

    case 2: // pairing request
    {
#ifndef ESPNOW_BRIDGE

        registerBridgeMac(mac, incomingData[1]);

#endif
    }
    break;

    case 3: // pairing response
    {
#ifdef ESPNOW_BRIDGE
        // DBG("[ESPNow] Pairing response received");
        registerDevice(mac);
#endif
    }
    break;

    case 4: // remote wakeup
    {
#ifndef ESPNOW_BRIDGE
        wakeUpReceived = true;
#endif
    }
    break;

    default:
        break;
    }
}

void ESPNowComponent::processMessage(const uint8_t *incomingData, int len)
{
    if (len < 3)
        return;

    uint8_t startID = incomingData[1];
    uint8_t endID = incomingData[2];

    // NDBG("Processing message for IDs " + String(startID) + " to " + String(endID));

    if (startID == 255 || endID == 255)
    {
        // message for all devices
    }
    else
    {
#ifndef ESPNOW_BRIDGE
        if (SettingsComponent::instance->propID < startID || SettingsComponent::instance->propID > endID)
        {
            NDBG("Message not for us, Id " + String(SettingsComponent::instance->propID));
            // not for us
            return;
        }
#endif
    }

    uint8_t addressLength = incomingData[3];

    if (len < 4 + addressLength)
    {
        NDBG("Address length more than data received");
        return;
    }

    String address = String((char *)(incomingData + 4), addressLength);

    // convert from OSC style to internal
    address = StringHelpers::oscPathToSerial(address);

#ifdef ESPNOW_BRIDGE
    if (startID == endID && startID != -1)
    {
        // DBG("Message from device ID " + String(startID));
        address = "dev." + String(startID) + "." + address;
    }
#endif

    uint8_t commandLength = incomingData[4 + addressLength];
    if (len < 5 + addressLength + commandLength)
    {
        NDBG("Command length more than data received");
        return;
    }

    String command = String((char *)(incomingData + 5 + addressLength), commandLength);

    var data[10];
    data[0] = address;
    data[1] = command;

    // NDBG("Message received with  " + address + ", " + command + " : " + String(len) + " bytes");

    int dataIndex = 5 + addressLength + commandLength;

    int dataCount = 2;
    while (dataIndex < len)
    {
        if (dataCount >= 10)
        {
            NDBG("Too many data received");
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

    // NDBG("[ESPNow] Message received from " + address + " : " + command + " : " + String(dataCount - 2) + " arguments : ");
    // for (int i = 0; i < dataCount; i++)
    // {
    //     NDBG(" > " + data[i].stringValue());
    // }

    bool doSendEvent = true;
#ifdef ESPNOW_BRIDGE
    doSendEvent = acceptCommands;
#endif
    if (doSendEvent)
        sendEvent(MessageReceived, data, dataCount);
}

void ESPNowComponent::sendMessage(int id, const String &address, const String &command, var *data, int numData)
{

    if (!enabled || !espNowInitialized)
        return;
    // Data format for message
    //  0 - Message
    //  1 - Broadcast Start ID
    //  2 - Broadcast End ID
    //  3 - Address length
    //  4 - Address
    //  5 - Command length
    //  6 - Command
    //  for each var
    //  7 - var type
    //  8 - var length (if string or ptr)
    //  9 - var data

    // NDBG("Sending message to " + mac + " : " + address + " : " + command);

#ifdef ESPNOW_BRIDGE
    uint8_t startID = broadcastStartID;
    uint8_t endID = broadcastEndID;
#else
    if (!bridgeInit)
        return;

    uint8_t startID = SettingsComponent::instance->propID;
    uint8_t endID = SettingsComponent::instance->propID;
#endif

    if (id != -1)
    {
        startID = id;
        endID = id;
    }

    // NDBG("Sending message to IDs " + String(startID) + " to " + String(endID) + " : " + address + " : " + command);

    sendPacketData[0] = 0; // Message
    sendPacketData[1] = startID;
    sendPacketData[2] = endID;
    sendPacketData[3] = address.length();
    memcpy(sendPacketData + 4, address.c_str(), address.length());
    sendPacketData[4 + address.length()] = command.length();
    memcpy(sendPacketData + 5 + address.length(), command.c_str(), command.length());

    int dataIndex = 5 + address.length() + command.length();
    for (int i = 0; i < numData; i++)
    {
        sendPacketData[dataIndex++] = data[i].type;

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
    String targetAddress = StringHelpers::serialPathToOSC(data[0].stringValue());

    bool shouldSend = false;
    int id = -1;

    if (targetAddress.startsWith("/dev/"))
    {
        int idEnd = targetAddress.indexOf('/', 5);
        if (idEnd != -1)
        {
            id = targetAddress.substring(5, idEnd).toInt();
            targetAddress = targetAddress.substring(idEnd + 1);
            shouldSend = true;
        }
    }
    else if (routeAll)
    {
        // remove all settings and comm to routeAll
        if (!targetAddress.startsWith("/settings") && !targetAddress.startsWith("/comm") && !targetAddress.startsWith("/wifi"))
            shouldSend = true;
    }

    if (shouldSend)
        sendMessage(id, targetAddress, data[1].stringValue(), &data[2], numData - 2);
}

void ESPNowComponent::sendStream(int id, int universe, Color3 *colors, int numColors)
{
    // Data format for stream
    //  0 - Stream (1)
    //  1-2 - Universe
    //  3-4 - Start Channel
    //  for each color
    //  R
    //  G
    //  B

    const int maxColorsPerPacket = 80; // 250 bytes max packet size. 80*3 + 7 = 247 bytes.
    const int colorsPerUniverse = 170;
    const int headerSize = 5;

    int colorsSent = 0;
    DBG("Streaming now, numColors : " + String(numColors) + ", universe : " + String(universe));

    while (colorsSent < numColors)
    {
        int currentUniverse = universe + floor((float)colorsSent / colorsPerUniverse);
        int startColorIndex = colorsSent % colorsPerUniverse;
        int startDmxChannel = 1 + startColorIndex * 3;

        // DBG("Preparing to send stream packet, colorsSent : " + String(colorsSent) + ", currentUniverse : " + String(currentUniverse) + ", startColorIndex : " + String(startColorIndex) + ", startDmxChannel : " + String(startDmxChannel));

        int remainingInUniverse = colorsPerUniverse - startColorIndex;
        int remainingToSend = numColors - colorsSent;

        int colorsInThisPacket = min({maxColorsPerPacket, remainingInUniverse, remainingToSend});

        int dataLen = headerSize + colorsInThisPacket * 3;
        if (dataLen > 250)
        {
            DBG("Stream packet too large");
            return;
        }

        sendPacketData[0] = 1; // Stream
        sendPacketData[1] = (currentUniverse >> 8) & 0xFF;
        sendPacketData[2] = currentUniverse & 0xFF;
        sendPacketData[3] = (startDmxChannel >> 8) & 0xFF;
        sendPacketData[4] = startDmxChannel & 0xFF;

        // DBG("Sending stream to ID " + String(id) + ", universe " + String(currentUniverse) + ", start channel " + String(startDmxChannel) + ", start color index : " + String(startColorIndex) + ", colors " + String(colorsInThisPacket));
        memcpy(sendPacketData + headerSize, colors + colorsSent, colorsInThisPacket * 3);

        sendPacket(id, sendPacketData, dataLen);

        colorsSent += colorsInThisPacket;
    }
}
#endif

void ESPNowComponent::sendPacket(int id, const uint8_t *data, int len)
{

#ifdef ESPNOW_BRIDGE

    if (broadcastMode)
    {
        esp_now_send(broadcastMac, data, len);
    }
    else
    {
        if (id == -1)
        {
            for (int i = 0; i < numConnectedDevices; i++)
            {
                // DBG("Sending to " + StringHelpers::macToString(remoteMacsBytes[i]));
                esp_now_send(remoteMacsBytes[i], data, len);
                if (i % 3 == 0)
                    delayMicroseconds(100);
            }
        }
        else if (id >= 0 && id < numConnectedDevices)
        {
            // DBG("Sending to " + StringHelpers::macToString(remoteMacsBytes[id]));
            esp_now_send(remoteMacsBytes[id], data, len);
        }
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

void ESPNowComponent::registerDMXReceiver(ESPNowDMXReceiver *receiver)
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

void ESPNowComponent::unregisterDMXReceiver(ESPNowDMXReceiver *receiver)
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

#ifdef ESPNOW_BRIDGE
#if defined USE_DMX || defined USE_ARTNET
void ESPNowComponent::onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len)
{
    sendStream(-1, universe, (Color3 *)data, len / 3);
}
#endif

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
        setupLongRange(broadcastMac);

        if (optimalRange)
        {
            NDBG("Setting optimal rate for broadcast peer");
            esp_now_rate_config_t rate_config = {
                .phymode = WIFI_PHY_MODE_11B,
                .rate = WIFI_PHY_RATE_1M_L,
                .ersu = false,
                .dcm = false};
            esp_now_set_peer_rate_config(broadcastMac, &rate_config);
        }
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
        if (remoteMacs[i] == address)
        {
            // DBG("Device already registered : " + address);
            return;
        }
    }

    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        if ((remoteMacs[i]).isEmpty())
        {
            remoteMacs[i] = address;
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

    if (result != ESP_OK)
        return;

    numConnectedDevices++;

    setupLongRange(deviceMac);
}
void ESPNowComponent::clearDevices()
{
    uint8_t *mac = (uint8_t *)malloc(6);

    for (int i = 0; i < ESPNOW_MAX_DEVICES; i++)
    {
        if (remoteMacs[i] == "")
            continue;

        DBG("Remove device " + remoteMacs[i]);

        StringHelpers::macFromString(remoteMacs[i], mac);

        esp_now_del_peer(mac);
        remoteMacs[i] = "";
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

    setupLongRange(bridgeMac);
    if (optimalRange)
    {
        NDBG("Setting optimal rate for bridge peer");
        esp_now_rate_config_t rate_config = {
            .phymode = WIFI_PHY_MODE_11B,
            .rate = WIFI_PHY_RATE_1M_L,
            .ersu = false,
            .dcm = false};
        esp_now_set_peer_rate_config(bridgeMac, &rate_config);

        NDBG("Optimal rate set for bridge peer");
    }

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

void ESPNowComponent::setupLongRange(const uint8_t *deviceMac)
{
    if (longRange)
    {
        uint8_t protocol = WIFI_PROTOCOL_LR;
        if (WifiComponent::instance->isUsingWiFi())
            protocol |= WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;

        esp_err_t pResult = esp_wifi_set_protocol(WIFI_IF_STA, protocol);
        if (pResult == ESP_OK)
            NDBG("Long range protocol set");
        else
            NDBG("Error setting long range protocol : " + String(pResult));

        esp_now_rate_config_t cfg = {};
        cfg.rate = WIFI_PHY_RATE_LORA_250K; // or WIFI_PHY_RATE_LORA_500K, or WIFI_PHY_RATE_1M_L
        esp_err_t lrResult = esp_now_set_peer_rate_config(deviceMac, &cfg);
        if (lrResult == ESP_OK)
            NDBG("Long range mode set for device");
        else
            NDBG("Error setting long range mode for device : " + String(lrResult));
    }
}

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
