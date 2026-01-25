#include "UnityIncludes.h"

Preferences Settings::prefs;
JsonDoc Settings::settings;

bool Settings::loadSettings()
{
    prefs.begin("blip");

    size_t settingsSize = prefs.getBytesLength("settings");
    char *bytes = (char *)malloc(settingsSize);
    prefs.getBytes("settings", bytes, settingsSize);

    DeserializationError err = deserializeMsgPack(settings, bytes);
    if (err)
    {
        DBG("Loading settings.json error : " + std::string(err.c_str()));
        return false;
    }

    settings["test"] = "super";
    DBG("Settings loaded " + std::to_string(settingsSize));

    return true;
}

bool Settings::saveSettings()
{
    size_t settingsSize = measureMsgPack(settings);
    char *bytes = (char *)malloc(settingsSize);
    size_t s = serializeMsgPack(settings, bytes, settingsSize);

    prefs.clear();
    if (s == 0)
    {
        DBG("Saving settings error");
        return false;
    }

    // if(prefs.isKey("settings.json")) prefs.remove("settings.json");
    prefs.putBytes("settings", bytes, s);

    DBG("Settings saved.");
    free(bytes);

    return true;
}

bool Settings::clearSettings()
{
    prefs.clear();
    settings.clear();
    DBG("Settings cleared.");
    return true;
}