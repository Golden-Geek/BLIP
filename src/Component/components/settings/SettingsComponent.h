#pragma once

#ifndef DEVICE_TYPE
#define DEVICE_TYPE "BLIP"
#endif

#ifndef DEVICE_NAME
#define DEVICE_NAME DEVICE_TYPE
#endif

#ifdef USE_POWER
#ifndef POWER_WAKEUP_BUTTON
#define POWER_WAKEUP_BUTTON -1
#endif
#ifndef POWER_WAKEUP_BUTTON_STATE
#define POWER_WAKEUP_BUTTON_STATE true
#endif
#endif

DeclareComponentSingleton(Settings, "settings", )

    DeclareIntParam(propID, 0);
DeclareStringParam(deviceName, DEVICE_TYPE);
DeclareStringParam(deviceType, DEVICE_TYPE);       // read-only
DeclareStringParam(firmwareVersion, BLIP_VERSION); // read-only

#ifdef USE_POWER
DeclareIntParam(wakeUpButton, POWER_WAKEUP_BUTTON);
DeclareBoolParam(wakeUpState, POWER_WAKEUP_BUTTON_STATE);
#endif

void setupInternal(JsonObject o) override;
void updateInternal() override;

bool handleCommandInternal(const std::string &command, var *data, int numData) override;

void saveSettings();
void clearSettings(bool keepWifiSettings = true);
void factoryReset() { clearSettings(false); }

std::string getDeviceID() const;
uint16_t getDeviceIDNum() const;
void getDeviceMac(byte *mac) const;

EndDeclareComponent

    // Manager
