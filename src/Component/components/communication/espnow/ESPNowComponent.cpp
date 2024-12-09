#include "UnityIncludes.h"

ImplementSingleton(ESPNowComponent);

void ESPNowComponent::setupInternal(JsonObject o)
{
    peerInfo.channel = 0;
    AddIntParamConfig(channel);

#ifdef ESPNOW_BRIDGE
    AddStringParamConfig(remoteMac);
    AddStringParamConfig(remoteMac2);
#endif

    hasReceivedData = false;
    lastSendTime = 0;
    lastReceiveTime = 0;
    strobe = false;

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

    uint8_t remoteMacArr2[6];
    sscanf(remoteMac2.c_str(), "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx",
           &remoteMacArr2[0], &remoteMacArr2[1], &remoteMacArr2[2],
           &remoteMacArr2[3], &remoteMacArr2[4], &remoteMacArr2[5]);

    memcpy(peerInfo2.peer_addr, remoteMacArr2, 6);
    esp_now_add_peer(&peerInfo2);

    NDBG("ESP-NOW initialized as bridge");

#else

#endif

    esp_now_register_recv_cb(&ESPNowComponent::onDataReceived);
    NDBG("ESP-NOW accessible at  : " + SettingsComponent::instance->getDeviceID());

    esp_now_register_send_cb(&ESPNowComponent::onDataSent);
    return true;
}

void ESPNowComponent::updateInternal()
{
#ifdef ESPNOW_BRIDGE
    unsigned long currentTime = millis();

    if (currentTime - lastSendTime >= 4)
    {
        lastSendTime = currentTime;
        // var data[4];
        // data[0] = 8;
        // data[1] = 154.5264f;
        // data[2] = String("ping");

        // uint8_t *testData = (uint8_t *)malloc(3);
        // testData[0] = 10;
        // testData[1] = 5;
        // testData[2] = 2;
        // data[3] = var(testData, 3);

        // DBG("Sending ping";
        // sendMessage("", "root", "log", data, 4);
        // free(testData);

        Color colors[32];
        for (int i = 0; i < 32; i++)
        {
            colors[i] = Color::HSV(millis()/2000.f + i / 32.f, (cos(i * millis() / 32000.f) * .5f + .5f), (int)strobe);
        }

        strobe = !strobe;

        // DBG("Sending stream");
        sendStream(remoteMac, 0, colors, 32);
    }
#endif
}

void ESPNowComponent::clearInternal()
{
}

void ESPNowComponent::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
        return;

    DBG("[ESPNow] Error sending to " + String(mac_addr[0]) + ":" + String(mac_addr[1]) + ":" + String(mac_addr[2]) + ":" + String(mac_addr[3]) + ":" + String(mac_addr[4]) + ":" + String(mac_addr[5]));
}

void ESPNowComponent::onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    // DBG("[ESPNow] Data received from " + String(mac[0]) + ":" + String(mac[1]) + ":" + String(mac[2]) + ":" + String(mac[3]) + ":" + String(mac[4]) + ":" + String(mac[5]) + " : " + String(len) + " bytes");

    instance->hasReceivedData = true;
    instance->lastReceiveTime = millis();

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

        // test send "ok" feedback

#ifndef ESPNOW_BRIDGE
        var feedbackData[1];
        feedbackData[0] = "ok";

        memcpy(instance->peerInfo.peer_addr, (void *)mac, 6);
        esp_err_t t = esp_now_add_peer(&instance->peerInfo);

        // DBG("Sending feedback to " + String(mac[0]) + ":" + String(mac[1]) + ":" + String(mac[2]) + ":" + String(mac[3]) + ":" + String(mac[4]) + ":" + String(mac[5]));

        ESPNowComponent::instance->sendMessage("", "root", "log", feedbackData, 1);
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
        break;
    }
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


    uint8_t totalLen = 9 + numColors * 3;
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
