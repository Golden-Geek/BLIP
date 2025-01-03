#pragma once

class StringHelpers
{
public:
    static void processStringMessage(const String &buffer, std::function<void(var *data, int numData)> callback);

    static String lowerCamelToTitleCase(String input);

    static String ipToString(IPAddress ip);

    static void macFromString(const String &mac, uint8_t *outMac);

    static String macToString(const uint8_t *mac);
};