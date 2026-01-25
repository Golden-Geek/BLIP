#include "UnityIncludes.h"

ImplementSingleton(SettingsComponent);

void SettingsComponent::setupInternal(JsonObject o)
{
    AddFunctionTrigger(saveSettings);
    AddFunctionTrigger(clearSettings);
    AddIntParamConfig(propID);
    AddStringParamConfig(deviceName);
    AddStringParamConfig(deviceType);
    AddStringParamConfig(firmwareVersion);
#ifdef USE_POWER
    AddIntParamConfig(wakeUpButton);
    AddBoolParamConfig(wakeUpState);
#endif
}

void SettingsComponent::updateInternal()
{
}

bool SettingsComponent::handleCommandInternal(const std::string &command, var *data, int numData)
{
    if (command == "save")
    {
        saveSettings();
        return true;
    }
    else if (command == "show")
    {
        std::string test;
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
    RootComponent::instance->fillSettingsData(o);
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

std::string SettingsComponent::getDeviceID() const
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

    std::string d = "";
    for (int i = 0; i < 6; i++)
        d += (i > 0 ? ":" : "") + StringHelpers::byteToHexString(mac[i]);

    d = StringHelpers::toUpperCase(d);
    return d;
}
