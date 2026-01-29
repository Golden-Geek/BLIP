#include "UnityIncludes.h"
#include "SettingsComponent.h"

ImplementSingleton(SettingsComponent);

void SettingsComponent::setupInternal(JsonObject o)
{
    AddFunctionTrigger(saveSettings);
    AddFunctionTrigger(clearSettings);
    AddFunctionTrigger(factoryReset);
    AddIntParamConfig(propID);
    AddStringParamConfig(deviceName);
    AddStringParamFeedback(deviceType);
    AddStringParamFeedback(firmwareVersion);
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
        //pretty print json
        //add new lines where there are { and }
        std::string pretty;
        pretty.reserve(test.size() * 2);
        for (char c : test)
        {
            pretty += c;
            if (c == '{' || c == '}')
            pretty += '\n';
        }
        NDBG(pretty.c_str());
        return true;
    }
    else if (command == "clear")
    {
        bool keepWifi = numData > 0 ? data[0].boolValue() : true;
        clearSettings(keepWifi);
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

void SettingsComponent::clearSettings(bool keepWifiSettings)
{

    Settings::clearSettings();

    if (keepWifiSettings)
    {
        JsonObject o = Settings::settings.to<JsonObject>();
        JsonObject comps = o.createNestedObject("components");
        RootComponent::instance->wifi.fillSettingsData(comps.createNestedObject(RootComponent::instance->wifi.name));
        Settings::saveSettings();
    }

    NDBG("Settings cleared, will reboot now.");
    delay(200);
    RootComponent::instance->restart();
}

std::string SettingsComponent::getDeviceID() const
{
    byte mac[6]{0, 0, 0, 0, 0, 0};
    getDeviceMac(mac);

    std::string d = "";
    for (int i = 0; i < 6; i++)
        d += (i > 0 ? ":" : "") + StringHelpers::byteToHexString(mac[i]);

    d = StringHelpers::toUpperCase(d);
    return d;
}

uint16_t SettingsComponent::getDeviceIDNum() const
{
    byte mac[6]{0, 0, 0, 0, 0, 0};
    getDeviceMac(mac);

    return (uint16_t)(mac[4] << 8) | mac[5];
}

void SettingsComponent::getDeviceMac(byte *mac) const
{
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
}
