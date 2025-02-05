#include "UnityIncludes.h"

ImplementSingleton(LedStreamReceiverComponent);

void LedStreamReceiverComponent::setupInternal(JsonObject o)
{
#ifdef USE_ARTNET
    artnetIsInit = false;
    AddIntParam(receiveRate);
#endif
}

bool LedStreamReceiverComponent::initInternal()
{

#ifdef USE_ARTNET
    artnet.setArtDmxCallback(&LedStreamReceiverComponent::onDmxFrame);
#endif

    setupConnection();
    return true;
}

void LedStreamReceiverComponent::updateInternal()
{
#ifdef USE_ARTNET
    if (!artnetIsInit)
        return;

    long curTime = millis();
    if (curTime > lastReceiveTime + (1000 / max(receiveRate, 1)))
    {
        lastReceiveTime = curTime;
        int r = -1;
        while (r != 0)
        {
            r = artnet.read();
        }
    }
#endif
}

void LedStreamReceiverComponent::clearInternal()
{
    // artnet.stop(); //when it will be implemented
}

void LedStreamReceiverComponent::onEnabledChanged()
{
    setupConnection();
}

void LedStreamReceiverComponent::setupConnection()
{
#ifdef USE_ARTNET
    bool shouldConnect = enabled && WifiComponent::instance->state == WifiComponent::Connected;
    if (shouldConnect)
    {
        artnet.begin();
        NDBG("Artnet started");
        artnetIsInit = true;
    }
    else
    {
        artnet.stop();
        artnetIsInit = false;
    }
#endif

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
    if (enabled)
    {
        ESPNowComponent::instance->registerStreamReceiver(this);
    }
    else
    {
        ESPNowComponent::instance->unregisterStreamReceiver(this);
    }
#endif
#endif
}

#ifdef USE_ARTNET
void LedStreamReceiverComponent::onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data)
{
    dispatchStreamData(universe, data, length);
}
#endif

void LedStreamReceiverComponent::registerStreamListener(LedStreamListener *listener)
{
    streamListeners.push_back(listener);
}

void LedStreamReceiverComponent::unregisterStreamListener(LedStreamListener *listener)
{
    for (int i = 0; i < streamListeners.size(); i++)
    {
        if (streamListeners[i] == listener)
        {
            streamListeners.erase(streamListeners.begin() + i);
            return;
        }
    }
}

void LedStreamReceiverComponent::onStreamReceived(const uint8_t *data, int len)
{
    if (len < 4)
    {
        DBG("Not enough data received");
        return;
    }

    int universe = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    if (len <= 4)
    {
        DBG("Error parsing stream data, not enough data");
        return;
    }

    dispatchStreamData(universe, data + 4, len - 4);
}

void LedStreamReceiverComponent::dispatchStreamData(uint16_t universe, const uint8_t *data, uint16_t len)
{
    for (auto &listener : streamListeners)
    {
        listener->onLedStreamReceived(universe, data, len);
    }
}