#pragma once

#ifndef RF24_DEFAULT_CE_PIN
#define RF24_DEFAULT_CE_PIN 18
#endif

#ifndef RF24_DEFAULT_CS_PIN
#define RF24_DEFAULT_CS_PIN 9
#endif

#ifndef RF24_DEFAULT_IRQ_PIN
#define RF24_DEFAULT_IRQ_PIN 7
#endif

DeclareComponentWithPriority(FlowtoysConnect, "Flowtoys Connect", false, DMXListenerDerive) // need to put withpriority to use only one macro ref call because of DMXListenerDerive

#pragma pack(push, 1) // prevents memory alignment from disrupting the layout and size of the network packet
    struct SyncPacket
{
    uint16_t groupID;
    uint32_t padding;
    uint8_t lfo[4];
    uint8_t global_hue;
    uint8_t global_sat;
    uint8_t global_val;
    uint8_t global_speed;
    uint8_t global_density;
    uint8_t lfo_active : 1;
    uint8_t hue_active : 1;
    uint8_t sat_active : 1;
    uint8_t val_active : 1;
    uint8_t speed_active : 1;
    uint8_t density_active : 1;
    uint8_t reserved[2];
    uint8_t page;
    uint8_t mode;
    uint8_t adjust_active : 1;
    uint8_t wakeup : 1;
    uint8_t poweroff : 1;
    uint8_t force_reload : 1;
    uint8_t save : 1;
    uint8_t _delete : 1;
    uint8_t alternate : 1;

    std::string toString() const
    {
        return "GroupID: " + std::to_string(groupID) +
               " Page: " + std::to_string(page) +
               " Mode: " + std::to_string(mode) +
               " Hue Active: " + std::to_string(hue_active) +
               " Sat Active: " + std::to_string(sat_active) +
               " Val Active: " + std::to_string(val_active) +
               " Speed Active: " + std::to_string(speed_active) +
               " Density Active: " + std::to_string(density_active) +
               " Adjust Active: " + std::to_string(adjust_active) +
               " Padding: " + std::to_string(padding) +
               " Wake Up: " + std::to_string(wakeup) +
               " Power Off: " + std::to_string(poweroff) +
               " Force Reload: " + std::to_string(force_reload) +
               " Save: " + std::to_string(save) +
               " Delete: " + std::to_string(_delete) +
               " Alternate: " + std::to_string(alternate);
    }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct InvitePacket
{
    uint8_t byte1 = 0x50;
    uint8_t byte2 = 0x06;
    uint16_t groupID;
    uint8_t byte5 = 0x00;
    uint8_t byte6 = 0x00;
};
#pragma pack(pop)

// DeclareIntParam(cePin, RF24_DEFAULT_CE_PIN);
// DeclareIntParam(csPin, RF24_DEFAULT_CS_PIN);
// DeclareIntParam(irqPin, RF24_DEFAULT_IRQ_PIN);
DeclareIntParam(dmxAddress, 1);
DeclareBoolParam(pairingMode, false);
DeclareIntParam(page, 1);
DeclareIntParam(mode, 1);
DeclareBoolParam(adjustMode, false);
DeclareFloatParam(brightness, 1.0f);
DeclareFloatParam(hue, 0.0f);
DeclareFloatParam(saturation, 1.0f);
DeclareFloatParam(speed, 1.0f);
DeclareFloatParam(density, 1.0f);
DeclareBoolParam(lfoActive, false);
DeclareFloatParam(lfo1, 0.0f);
DeclareFloatParam(lfo2, 0.0f);
DeclareFloatParam(lfo3, 0.0f);
DeclareFloatParam(lfo4, 0.0f);
DeclareIntParam(groupID, 0);
DeclareBoolParam(enablePowerPress, true);


RF24 *radio = nullptr;

SyncPacket packet;
SyncPacket receivingPacket;
InvitePacket pairingPacket;

ButtonComponent* button = nullptr;
IOComponent* ledOutput = nullptr;
#ifdef USE_DIPSWITCH
DIPSwitchComponent* dipSwitch = nullptr;
#endif

long timeAtButtonDown = 0;
int pageOnRelease = 1;

long timeAtPairingStart = 0;

//power press
bool switchOnRelease = false;
bool wakeUpSwitchState = false;

// spinlock

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;

void clearInternal() override;

void checkButton();
void paramValueChangedInternal(ParamInfo *param) override;

void setNewGroupID();
void sendPacket();
void sendSyncPacket();
void sendPairingPacket();

void receivePacket();

void onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) override;
EndDeclareComponent