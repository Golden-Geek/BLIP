#include "UnityIncludes.h"

namespace
{
    volatile bool g_rf24IrqFired = false;
#if defined(ESP32)
    void IRAM_ATTR onRf24Irq()
#else
    void onRf24Irq()
#endif
    {
        g_rf24IrqFired = true;
    }
}

void FlowtoysConnectComponent::setupInternal(JsonObject o)
{
    DMXReceiverComponent::instance->registerDMXListener(this);

    updateRate = 40; // low fps but enough for this

    AddIntParamConfig(dmxAddress);
    AddBoolParam(pairingMode);
    AddBoolParamConfig(enablePowerPress);

    // AddIntParamConfig(cePin);
    // AddIntParamConfig(csPin);
    // AddIntParamConfig(irqPin);
    AddIntParamConfig(page);
    AddIntParamConfig(mode);
    AddBoolParamConfig(adjustMode);
    ParamInfo *bInfo = AddFloatParamConfig(brightness);
    bInfo->setRange(&defaultRange);
    ParamInfo *hInfo = AddFloatParamConfig(hue);
    hInfo->setRange(&defaultRange);
    ParamInfo *sInfo = AddFloatParamConfig(saturation);
    sInfo->setRange(&defaultRange);
    ParamInfo *spInfo = AddFloatParamConfig(speed);
    spInfo->setRange(&defaultRange);
    ParamInfo *dInfo = AddFloatParamConfig(density);
    dInfo->setRange(&defaultRange);
    AddBoolParamConfig(lfoActive);
    AddFloatParamConfig(lfo1);
    dInfo->setRange(&defaultRange);
    AddFloatParamConfig(lfo2);
    dInfo->setRange(&defaultRange);
    AddFloatParamConfig(lfo3);
    dInfo->setRange(&defaultRange);
    AddFloatParamConfig(lfo4);
    dInfo->setRange(&defaultRange);
    AddIntParamWithTag(groupID, TagConfig | TagFeedback);

    if (groupID == 0)
        setNewGroupID();
    else
        pairingPacket.groupID = (groupID & 0xFFFF);

    button = RootComponent::instance->buttons.items[0];
    ledOutput = RootComponent::instance->ios.items[0];
#ifdef USE_DIPSWITCH
    dipSwitch = &RootComponent::instance->dipswitch;
#endif

    NDBG("FlowtoysConnect: Setup complete, group ID " + std::to_string(pairingPacket.groupID));
}

bool FlowtoysConnectComponent::initInternal()
{

    uint8_t address[5] = {0x01, 0x07, 0xf1, 0, 0};

    if (radio != nullptr)
    {
        delete radio;
        radio = nullptr;
    }

    NDBG("FlowtoysConnect: Initializing RF24 on CE pin " + std::to_string(RF24_DEFAULT_CE_PIN) + ", CS pin " + std::to_string(RF24_DEFAULT_CS_PIN) + ", IRQ pin " + std::to_string(RF24_DEFAULT_IRQ_PIN));

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    SPI.begin(RF24_DEFAULT_SPI_SCK, RF24_DEFAULT_SPI_MISO, RF24_DEFAULT_SPI_MOSI, RF24_DEFAULT_CS_PIN);
#else
    SPI.begin();
#endif

    radio = new RF24(RF24_DEFAULT_CE_PIN, RF24_DEFAULT_CS_PIN);
    if (!radio)
        return false;

    radio->begin();
    radio->setAutoAck(false);
    radio->setDataRate(RF24_250KBPS);
    radio->setChannel(2);
    radio->setAddressWidth(3);
    radio->setPayloadSize(sizeof(SyncPacket));
    radio->setCRCLength(RF24_CRC_16);

    radio->stopListening();
    radio->openReadingPipe(1, address);
    radio->openWritingPipe(address);
    radio->startListening();

    if (RF24_DEFAULT_IRQ_PIN >= 0)
    {
        pinMode(RF24_DEFAULT_IRQ_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(RF24_DEFAULT_IRQ_PIN), onRf24Irq, FALLING);
    }

    radio->printDetails();

    return radio->isChipConnected();
}

void FlowtoysConnectComponent::updateInternal()
{
    if (!isInit)
        return;

    if (millis() > 2000)
    { // wait 2 seconds after init to avoid fake button values
        checkButton();
    }

    if (pairingMode && timeAtPairingStart > 0 && millis() - timeAtPairingStart > 60000)
    {
        SetParam(pairingMode, false);
        NDBG("FlowtoysConnect: Exiting pairing mode due to 1min timeout");
    }

#ifdef USE_DIPSWITCH
    int dipSwitchValue = dipSwitch->value;
    if (dipSwitchValue != dmxAddress)
    {
        NDBG("FlowtoysConnect: DIP Switch changed, new DMX Address: " + std::to_string(dipSwitchValue));
        SetParam(dmxAddress, dipSwitchValue);
    }
#endif

    sendPacket();

    if (RF24_DEFAULT_IRQ_PIN >= 0)
    {
        if (g_rf24IrqFired)
        {
            g_rf24IrqFired = false;
            receivePacket();
        }
    }
    else
    {
        receivePacket();
    }
}

void FlowtoysConnectComponent::clearInternal()
{
    DMXReceiverComponent::instance->unregisterDMXListener(this);

    if (RF24_DEFAULT_IRQ_PIN >= 0)
    {
        detachInterrupt(digitalPinToInterrupt(RF24_DEFAULT_IRQ_PIN));
    }
}

void FlowtoysConnectComponent::checkButton()
{
    if (!pairingMode)
    {
        bool ledValue = button->value && !switchOnRelease;
        SetComponentParam(ledOutput, value, ledValue * 0.5f);
        if (button->veryLongPress)
        {
            SetParam(pairingMode, true);
            NDBG("FlowtoysConnect: Entering pairing mode");
        }
        else if (button->multiPressCount >= 2)
        {

            pageOnRelease = button->multiPressCount;
        }
        else if (button->multiPressCount == 0 && pageOnRelease > 0)
        {
            if (pageOnRelease >= 6 && pageOnRelease <= 12)
            {
                pageOnRelease = 1;
            }
            else if (pageOnRelease > 12)
            {
                pageOnRelease = 13;
            }

            SetParam(page, pageOnRelease);
            SetParam(mode, 1);
            pageOnRelease = 0;

            NDBG("FlowtoysConnect: Setting page to " + std::to_string(page));
        }
        else
        {
            if (button->value)
            {
                if (timeAtButtonDown == 0)
                    timeAtButtonDown = millis();

                if (enablePowerPress && button->longPress)
                {
                    switchOnRelease = true;
                }
            }
            else
            {
                if (!button->value)
                {
                    if (millis() - timeAtButtonDown < SHORTPRESS_TIME)
                    {
                        SetParam(mode, (mode + 1) % 10);
                        NDBG("FlowtoysConnect: Setting mode to " + std::to_string(mode) + " on short press");
                    }
                }
                if (enablePowerPress && switchOnRelease)
                {
                    wakeUpSwitchState = !wakeUpSwitchState;
                    switchOnRelease = false;
                }
                timeAtButtonDown = 0;
            }
        }
    }
    else
    {

        float blinkValue = ((millis() / 250) % 2) ? 1.0f : 0.0f;
        SetComponentParam(ledOutput, value, blinkValue);

        if (button->value)
        {
            if (!button->longPress && !button->veryLongPress)
            {
                SetParam(pairingMode, false);
                SetComponentParam(ledOutput, value, 0.0f);
                NDBG("FlowtoysConnect: Exiting pairing mode");
            }
        }
    }
}

void FlowtoysConnectComponent::paramValueChangedInternal(ParamInfo *paramInfo)
{
    if (paramInfo->ptr == &pairingMode)
    {
        radio->stopListening();
        if (pairingMode)
        {

            timeAtPairingStart = millis();
            setNewGroupID();
            radio->setPayloadSize(sizeof(InvitePacket));
        }
        else
        {
            timeAtPairingStart = 0;
            radio->setPayloadSize(sizeof(SyncPacket));
        }

        radio->startListening();
    }
}

void FlowtoysConnectComponent::setNewGroupID()
{
    int newGroupID = random(1, 65535);
    SetParam(groupID, newGroupID);
    pairingPacket.groupID = (groupID & 0xFFFF);
    SettingsComponent::instance->saveSettings();
}

void FlowtoysConnectComponent::sendPacket()
{
    radio->stopListening();
    if (pairingMode)
    {
        sendPairingPacket();
    }
    else
    {
        sendSyncPacket();
    }

    radio->startListening();
}

void FlowtoysConnectComponent::sendSyncPacket()
{
    packet.global_hue = (uint8_t)(hue * 255);
    packet.global_sat = (uint8_t)(saturation * 255);
    packet.global_val = (uint8_t)(brightness * 255);
    packet.global_speed = (uint8_t)(speed * 255);
    packet.global_density = (uint8_t)(density * 255);

    packet.hue_active = 1;
    packet.sat_active = 1;
    packet.val_active = 1;
    packet.speed_active = 1;
    packet.density_active = 1;

    uint8_t p = page - 1;
    if (p < 0)
        p = 0;
    if (p > 16)
        p = 16;
    packet.page = p;

    uint8_t m = mode - 1;
    if (m < 0)
        m = 0;
    packet.mode = m;

    uint8_t act = adjustMode ? 1 : 0;
    packet.adjust_active = act;
    packet.density_active = act;
    packet.speed_active = act;
    packet.hue_active = act;
    packet.sat_active = act;
    packet.val_active = act;
    packet.lfo_active = lfoActive ? 1 : 0;
    packet.lfo[0] = (uint8_t)(lfo1 * 255);
    packet.lfo[1] = (uint8_t)(lfo2 * 255);
    packet.lfo[2] = (uint8_t)(lfo3 * 255);
    packet.lfo[3] = (uint8_t)(lfo4 * 255);

    // set group ID
    // reverse group id bytes for some reason
    uint16_t reverseGroupID = ((groupID & 0xFF) << 8) | ((groupID >> 8) & 0xFF);
    packet.groupID = reverseGroupID;
    packet.padding++;

    // NDBG("Send page " + std::to_string(packet.page) + " mode " + std::to_string(packet.mode));
    radio->write(&packet, sizeof(SyncPacket));

    // NDBG("Sent Sync Packet: " + packet.toString());
}

void FlowtoysConnectComponent::sendPairingPacket()
{
    radio->write(&pairingPacket, sizeof(InvitePacket));
    // show 2 bytes of the packet group ID
    // NDBG("Sent Pairing Packet for group " + std::to_string(pairingPacket.groupID & 0xFF) + ", " + std::to_string((pairingPacket.groupID >> 8) & 0xFF));
}

void FlowtoysConnectComponent::receivePacket()
{
    while (radio->available())
    {
        // Can receive either SyncPacket or InvitePacket

        int payloadSize = radio->getDynamicPayloadSize();

        bool isSyncPacket = (payloadSize == sizeof(SyncPacket));
        bool isInvitePacket = (payloadSize == sizeof(InvitePacket));
        bool goodPayload = (isSyncPacket && !pairingMode) || (isInvitePacket && pairingMode);

        if (!goodPayload)
        {
            // NDBG("FlowtoysConnect: Bad payload size " + std::to_string(payloadSize));
            radio->flush_rx();
            continue;
        }

        if (payloadSize == sizeof(SyncPacket))
        {
            radio->read(&receivingPacket, sizeof(SyncPacket));
            if (receivingPacket.padding > packet.padding)
                packet.padding = receivingPacket.padding; // sync back the group ID to maintain connection
        }
        else if (payloadSize == sizeof(InvitePacket))
        {
            InvitePacket invitePacket;
            radio->read(&invitePacket, sizeof(InvitePacket));
        }
        else
        {
            // unknown packet size
            uint8_t buffer[32];
            radio->read(&buffer, payloadSize);
            // NDBG("Unknown packet received of size " + std::to_string(payloadSize));
        }
    }
}

void FlowtoysConnectComponent::onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len)
{
    if (dmxAddress == 0)
    {
        return; // ignore if address is 0
    }

    uint16_t addr = dmxAddress;
    if (len + startChannel < addr)
        return; // not in range

    if (startChannel > addr + 1)
        return; // not in range

    if (len + startChannel >= addr + 13)
    {
        int chStart = addr - startChannel;
        page = data[chStart];
        mode = data[chStart + 1];
        adjustMode = data[chStart + 2] > 0;
        hue = data[chStart + 3];
        saturation = data[chStart + 4];
        brightness = data[chStart + 5];
        speed = data[chStart + 6];
        density = data[chStart + 7];
        lfoActive = data[chStart + 8] > 0;
        lfo1 = data[chStart + 9];
        lfo2 = data[chStart + 10];
        lfo3 = data[chStart + 11];
        lfo4 = data[chStart + 12];
    }
    
    NDBG("Received DMX Packet " + std::to_string(data[0]) + ", " + std::to_string(data[1]) + ", " + std::to_string(data[2]) + ", " + std::to_string(data[3]) + ", " + std::to_string(data[4]) + ", " + std::to_string(data[5]) + ", " + std::to_string(data[6]) + ", " + std::to_string(data[7]) + ", " + std::to_string(data[8]) + ", " + std::to_string(data[9]) + ", " + std::to_string(data[10]) + ", " + std::to_string(data[11]) + ", " + std::to_string(data[12]));
}
