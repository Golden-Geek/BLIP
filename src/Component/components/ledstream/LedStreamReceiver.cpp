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

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
    if (ESPNowComponent::instance->enabled)
        return;
#endif

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
            // DBG("Receiving artnet, returned " + String(r));
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

#ifdef USE_ARTNET

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
    if (ESPNowComponent::instance->enabled) // in this case, a a node with ESPNow enabled should not try to receive artnet directly
    {
        // NDBG("ESPNow enabled, disabling artnet");
        return;
    }
#endif

    bool shouldConnect = enabled && WifiComponent::instance->state == WifiComponent::Connected;
    if (shouldConnect)
    {
        artnet.begin();
        artnetIsInit = true;
    }
    else
    {
        artnet.stop();
        artnetIsInit = false;
    }
#endif
}

#ifdef USE_ARTNET
void LedStreamReceiverComponent::onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data)
{
    RootComponent::instance->timeAtLastSignal = millis();
    instance->dispatchStreamData(universe, data, 1, length);
}
#endif


#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
void LedStreamReceiverComponent::onStreamReceived(const uint8_t *data, int len)
{    
    if (len < 4)
    {
        DBG("Not enough data received");
        return;
    }

    uint16_t universe = (data[0] << 8) | data[1];
    uint16_t startChannel = (data[2] << 8) | data[3];

    // DBG("Received stream data for universe " + String(universe) + " starting at channel " + String(startChannel) + " with " + String(len - 4) + " bytes");
    
    if (len <= 4)
    {
        DBG("Error parsing stream data, not enough data");
        return;
    }

    dispatchStreamData(universe, data + 4, startChannel, len - 4);
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

void LedStreamReceiverComponent::dispatchStreamData(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len)
{
    // DBG("Dispatching stream data for universe " + String(universe) + " starting at channel " + String(startChannel) + " with " + String(len) + " bytes to " + String(streamListeners.size()) + " listeners");
    for (auto &listener : streamListeners)
    {
        listener->onLedStreamReceived(universe, data, startChannel, len);
    }
}
