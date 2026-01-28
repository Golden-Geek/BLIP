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

#define MAX_GROUPS 4

DeclareComponent(FlowtoysConnect, "Flowtoys Connect", )

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
};
#pragma pack(pop)

// DeclareIntParam(cePin, RF24_DEFAULT_CE_PIN);
// DeclareIntParam(csPin, RF24_DEFAULT_CS_PIN);
// DeclareIntParam(irqPin, RF24_DEFAULT_IRQ_PIN);
DeclareBoolParam(pairingMode, false);
DeclareIntParam(page, 1);
DeclareIntParam(mode, 1);
DeclareBoolParam(adjustMode, false);
DeclareFloatParam(brightness, 1.0f);
DeclareFloatParam(hue, 0.0f);
DeclareFloatParam(saturation, 1.0f);
DeclareFloatParam(speed, 1.0f);
DeclareFloatParam(density, 1.0f);
DeclareIntParam(group1ID, 0);
DeclareIntParam(group2ID, 0);
DeclareIntParam(group3ID, 0);
DeclareIntParam(group4ID, 0);
int *groupIds[MAX_GROUPS] = {&group1ID, &group2ID, &group3ID, &group4ID};

RF24 *radio = nullptr;

SyncPacket packet;
SyncPacket receivingPacket;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;

void clearInternal() override;

void paramValueChangedInternal(ParamInfo *param) override;

void sendPacket();
void sendPacketToGroup(int groupID);

void receivePacket();

EndDeclareComponent