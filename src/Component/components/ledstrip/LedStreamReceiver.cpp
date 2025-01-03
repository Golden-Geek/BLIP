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

#if USE_LEDSTRIP
void LedStreamReceiverComponent::registerLayer(LedStripStreamLayer *layer)
{
    layers.push_back(layer);
}

void LedStreamReceiverComponent::unregisterLayer(LedStripStreamLayer *layer)
{
    for (int i = 0; i < layers.size(); i++)
    {
        if (layers[i] == layer)
        {
            layers.erase(layers.begin() + i);
            return;
        }
    }
}
#endif

#ifdef USE_ARTNET
void LedStreamReceiverComponent::onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data)
{
    // DBG("Received Artnet "+String(universe));
    instance->handleReceiveData(universe, length, data);
}
#endif

#ifdef USE_ESPNOW
#ifdef ESPNOW_BRIDGE
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
#else
void LedStreamReceiverComponent::onStreamReceived(const uint8_t *data, int len)
{

    // NDBG("Received Stream " + String(len));
    if (len < 4)
    {
        DBG("Not enough data received");
        return;
    }

    int universe = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    // int start = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

    int count = (len - 4) / 3;

    // NDBG("Received Stream universe : " + String(universe) + ", count : " + String(count) + ", start : " + String(start));

    if (count < 1)
    {
        DBG("No colors provided");
        return;
    }

    if (len < 4 + count * 3)
    {
        DBG("Led count more than provided data");
        return;
    }

    handleReceiveData(universe, count * 3, (uint8_t *)data + 4);
}
#endif
#endif

void LedStreamReceiverComponent::handleReceiveData(uint16_t universe, uint16_t length, uint8_t *data)
{
#ifdef USE_LEDSTRIP

    float multiplier = 1.0f;
    if (RootComponent::instance->isShuttingDown())
    {
        float relT = (millis() - RootComponent::instance->timeAtShutdown) / 1000.0f;
        const float animTime = 1.0f;
        multiplier = max(1 - relT * 2 / animTime, 0.f);
    }

    for (auto &layer : layers)
    {
        int numUniverses = std::ceil(layer->strip->count * 1.0f / 170); // 170 leds per universe
        if (universe < layer->universe || universe > layer->universe + numUniverses - 1)
            continue;

        // DBG("Received Artnet " + String(universe) + " " + String(length) + " " + String(sequence) + " " + String(stripIndex) + " " + String(layer->strip->count));

        int start = (universe - layer->universe) * 170;

        // DBG("Received Artnet " + String(universe) + ", start = " + String(start));
        for (int i = 0; i < layer->strip->count && i < 170 && (i * 3) < length; i++)
        {
            layer->colors[i + start] = Color(data[i * 3] * multiplier, data[i * 3 + 1] * multiplier, data[i * 3 + 2] * multiplier);
        }

        layer->lastReceiveTime = millis() / 1000.0f;
        layer->hasCleared = false;
        // memcpy((uint8_t *)layer->colors, streamBuffer + 1, byteIndex - 2);
    }
#endif

#if defined USE_ESPNOW && defined ESPNOW_BRIDGE

    // NDBG("Received Stream " + String(length) + " " + String(universe));
    for (auto &listener : streamListeners)
    {
        // NDBG("Send to listener");
        listener->onLedStreamReceived(universe, data, length);
    }

#endif
}