#pragma once

class StringHelpers
{
public:
    static void processStringMessage(const std::string &buffer, std::function<void(var *data, int numData)> callback);

    static std::string lowerCamelToTitleCase(std::string input);
    static std::string toLowerCase(const std::string &input);
    static std::string toUpperCase(const std::string &input);
    static void replaceAll(std::string &str, const std::string &from, const std::string &to);

    static void macFromString(const std::string &mac, uint8_t *outMac);
    static std::string macToString(const uint8_t *mac);

    static std::string byteToHexString(uint8_t byte);

    static std::string oscPathToSerial(const std::string& oscPath);
    static std::string serialPathToOSC(const std::string& serialPath);

};