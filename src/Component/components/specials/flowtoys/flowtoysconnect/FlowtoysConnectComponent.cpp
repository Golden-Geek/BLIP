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
    updateRate = 15; // low fps but enough for this

    AddBoolParam(pairingMode);
    // AddIntParamConfig(cePin);
    // AddIntParamConfig(csPin);
    // AddIntParamConfig(irqPin);
    AddIntParamConfig(page);
    AddIntParamConfig(mode);
    AddBoolParamConfig(adjustMode);
    AddFloatParamConfig(brightness);
    AddFloatParamConfig(hue);
    AddFloatParamConfig(saturation);
    AddFloatParamConfig(speed);
    AddFloatParamConfig(density);

    AddIntParamConfig(group1ID);
    AddIntParamConfig(group2ID);
    AddIntParamConfig(group3ID);
    AddIntParamConfig(group4ID);

    NDBG("FlowtoysConnect: Setup complete with first group ID " + std::to_string(group1ID));
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

    if (!pairingMode)
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
    if (RF24_DEFAULT_IRQ_PIN >= 0)
    {
        detachInterrupt(digitalPinToInterrupt(RF24_DEFAULT_IRQ_PIN));
    }

    if (radio != nullptr)
    {
        delete radio;
        radio = nullptr;
    }
}

void FlowtoysConnectComponent::paramValueChangedInternal(ParamInfo *paramInfo)
{
    if (paramInfo->ptr == &pairingMode)
    {
        if (pairingMode)
        {
            for (int i = 0; i < MAX_GROUPS; i++)
            {
                SetParam(*groupIds[i], 0);
            }
        }
        else
        {
            Settings::saveSettings();
        }
    }
}

void FlowtoysConnectComponent::sendPacket()
{
    radio->stopListening();

    for (int i = 0; i < 4; i++)
    {
        sendPacketToGroup(i);
    }

    radio->startListening();
}
void FlowtoysConnectComponent::sendPacketToGroup(int index)
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

    // set group ID
    int gid = *groupIds[index];
    packet.groupID = ((gid & 0xff) << 8) | ((gid >> 8) & 0xff);
    packet.padding++;

    // NDBG("Send page " + std::to_string(packet.page) + " mode " + std::to_string(packet.mode) + " to group " + std::to_string(gid));
    radio->write(&packet, sizeof(SyncPacket));
}

void FlowtoysConnectComponent::receivePacket()
{
    while (radio->available())
    {
        radio->read(&receivingPacket, sizeof(SyncPacket));

        receivingPacket.groupID = (receivingPacket.groupID >> 8 & 0xff) | ((receivingPacket.groupID & 0xff) << 8);

        if (pairingMode)
        {
            // NDBG("FlowtoysConnect: Received packet for group " + std::to_string(receivingPacket.groupID) + " page " + std::to_string(receivingPacket.page) + " mode " + std::to_string(receivingPacket.mode));
            if (receivingPacket.page == 1 && receivingPacket.mode == 0) // pairing request
            {
                bool alreadyPaired = false;
                // check if we are already paired
                for (int i = 0; i < MAX_GROUPS; i++)
                {
                    if (*groupIds[i] == receivingPacket.groupID)
                    {
                        alreadyPaired = true;
                        break; // already paired
                    }
                }

                if (alreadyPaired)
                    continue;

                // add to first available group slot
                for (int i = 0; i < MAX_GROUPS; i++)
                {
                    if (*groupIds[i] == 0)
                    {
                        NDBG("FlowtoysConnect: Paired with group " + std::to_string(receivingPacket.groupID) + " in slot " + std::to_string(i + 1));
                        SetParam(*groupIds[i], receivingPacket.groupID);
                        break;
                    }

                    if (i == MAX_GROUPS - 1)
                    {
                        // no available slots
                        NDBG("FlowtoysConnect: No available group slots for pairing");
                    }
                }
            }
            continue;
        }

        if (receivingPacket.groupID == *groupIds[0])
        {
            packet.padding = max(receivingPacket.padding, packet.padding);
        }
    }
}
