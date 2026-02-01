#include "UnityIncludes.h"

ImplementSingleton(DMXReceiverComponent);

void DMXReceiverComponent::setupInternal(JsonObject o)
{
    setCustomUpdateRate(60, o);

#ifdef USE_ARTNET
    artnetIsInit = false;
#endif
}

bool DMXReceiverComponent::initInternal()
{

    bool result = true;

#ifdef USE_ARTNET
    artnet.setArtDmxCallback(&DMXReceiverComponent::onDmxFrame);
#endif

    setupConnection();

#ifdef USE_DMX
    dmx.onReceive([this](uint8_t *data, int len)
                  {
                      unsigned long currentMillis = millis();
                      if (currentMillis - timeAtLastDMXUpdate < 25)
                          return; // debounce to avoid flooding with updates
                      timeAtLastDMXUpdate = currentMillis;
                      RootComponent::instance->timeAtLastSignal = millis();
                      dispatchDMXData(0, data + 1, 1, len - 1); // skip start code byte
                  });

    int dmxResult = dmx.begin(DMXMode::Receive, DMX_RECEIVE_PIN, DMX_OUTPUT_PIN);
    pinMode(DMX_ENABLE_PIN, OUTPUT);
    digitalWrite(DMX_ENABLE_PIN, LOW); // Enable DMX Receiver

    if (dmxActivityLedPin >= 0)
    {
        pinMode(dmxActivityLedPin, OUTPUT);
        digitalWrite(dmxActivityLedPin, LOW);
    }

    if (dmxResult < 0)
    {
        DBG("Failed to initialize DMX Receiver");
        result = false;
    }
#endif

    return result;
}

void DMXReceiverComponent::updateInternal()
{
#ifdef USE_ARTNET
    if (!artnetIsInit)
        return;

    int r = -1;
    while (r != 0)
    {
        r = artnet.read();
        // DBG("Receiving artnet, returned " + std::to_string(r));
    }

#endif

#ifdef USE_DMX
    if (dmxActivityLedPin >= 0)
    {
        unsigned long currentMillis = millis();
        bool ledState = (currentMillis - timeAtLastDMXUpdate) < 100;
        digitalWrite(dmxActivityLedPin, ledState ? HIGH : LOW);
    }
#endif
}

void DMXReceiverComponent::clearInternal()
{
    // artnet.stop(); //when it will be implemented
}

void DMXReceiverComponent::onEnabledChanged()
{
    setupConnection();
}

void DMXReceiverComponent::setupConnection()
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

#if defined USE_ARTNET
void DMXReceiverComponent::onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data)
{
    // DBG("On DMX Frame");
    RootComponent::instance->timeAtLastSignal = millis();
    instance->dispatchDMXData(universe, data, 1, length);
}
#endif

#if defined USE_ESPNOW && not defined ESPNOW_BRIDGE
void DMXReceiverComponent::onStreamReceived(const uint8_t *data, int len)
{
    if (len < 4)
    {
        DBG("Not enough data received");
        return;
    }

    uint16_t universe = (data[0] << 8) | data[1];
    uint16_t startChannel = (data[2] << 8) | data[3];

    // DBG("Received stream data for universe " + std::to_string(universe) + " starting at channel " + std::to_string(startChannel) + " with " + std::to_string(len - 4) + " bytes");

    if (len <= 4)
    {
        DBG("Error parsing stream data, not enough data");
        return;
    }

    dispatchDMXData(universe, data + 4, startChannel, len - 4);
}
#endif

void DMXReceiverComponent::registerDMXListener(DMXListener *listener)
{
    dmxListeners.push_back(listener);
}

void DMXReceiverComponent::unregisterDMXListener(DMXListener *listener)
{
    for (int i = 0; i < dmxListeners.size(); i++)
    {
        if (dmxListeners[i] == listener)
        {
            dmxListeners.erase(dmxListeners.begin() + i);
            return;
        }
    }
}

void DMXReceiverComponent::dispatchDMXData(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len)
{
    // DBG("Dispatching stream data for universe " + std::to_string(universe) + " starting at channel " + std::to_string(startChannel) + " with " + std::to_string(len) + " bytes to " + std::to_string(dmxListeners.size()) + " listeners");
    for (auto &listener : dmxListeners)
    {
        listener->onDMXReceived(universe, data, startChannel, len);
    }
}
