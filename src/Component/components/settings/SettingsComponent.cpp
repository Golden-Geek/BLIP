#include "UnityIncludes.h"

ImplementSingleton(SettingsComponent);

void SettingsComponent::setupInternal(JsonObject o)
{
    AddIntParam(propID);
    AddStringParam(deviceName);
    AddStringParamConfig(deviceType);
#ifdef USE_POWER
    AddIntParamConfig(wakeUpButton);
    AddBoolParamConfig(wakeUpState);
    AddIntParamConfig(shutdownChargeNoSignal); // seconds, 0 = disabled
#endif
}

void SettingsComponent::updateInternal()
{
#ifdef USE_POWER
    if (gotSignal)
        return;

    if (shutdownChargeNoSignal > 0 && !gotSignal)
    {
        #ifdef USE_BUTTON
        if(RootComponent::instance->buttons.count > 0)
        {
            if (RootComponent::instance->buttons.items[0]->wasPressedAtBoot) return; // do not shutdown if button was pressed at boot
        }
        #endif

        #ifdef USE_BATTERY
        if (BatteryComponent::instance->chargePin != -1 && !BatteryComponent::instance->charging) return; // only shutdown if charging
        #endif

        if(millis() > shutdownChargeNoSignal * 1000) {
            NDBG("No signal received for " + String(shutdownChargeNoSignal) + " seconds while charging, shutting down.");
            delay(200);
            RootComponent::instance->shutdown();
            gotSignal = true; // prevent multiple shutdown calls
        }
    }
#endif
}

bool SettingsComponent::handleCommandInternal(const String &command, var *data, int numData)
{
    if (command == "save")
    {
        saveSettings();
        return true;
    }
    else if (command == "show")
    {
        String test;
        serializeJson(Settings::settings, test);
        DBG(test);
        return true;
    }
    else if (command == "clear")
    {
        clearSettings();
        return true;
    }

    return false;
}

void SettingsComponent::saveSettings()
{
    Settings::settings.clear();
    JsonObject o = Settings::settings.to<JsonObject>();
    RootComponent::instance->fillSettingsData(o, true);
    Settings::saveSettings();
    NDBG("Settings saved");
}

void SettingsComponent::clearSettings()
{
    Settings::clearSettings();
    NDBG("Settings cleared, will reboot now.");
    delay(200);
    RootComponent::instance->restart();
}

String SettingsComponent::getDeviceID() const
{
    byte mac[6]{0, 0, 0, 0, 0, 0};

#ifdef USE_WIFI
#ifdef USE_ETHERNET
    if (WifiComponent::instance->isUsingEthernet())
        ETH.macAddress(mac);
    else
        WiFi.macAddress(mac);
#else
    WiFi.macAddress(mac);
#endif
#endif

    String d = "";
    for (int i = 0; i < 6; i++)
        d += (i > 0 ? ":" : "") + String(mac[i], HEX);

    d.toUpperCase();
    return d;
}
