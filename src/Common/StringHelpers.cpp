#include "UnityIncludes.h"
#include <string>
#include <functional>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include "StringHelpers.h"

void StringHelpers::processStringMessage(const std::string &buffer, std::function<void(var *data, int numData)> callback)
{
    size_t splitIndex = buffer.find(' ');

    std::string target = (splitIndex == std::string::npos) ? buffer : buffer.substr(0, splitIndex); // target component

    size_t tcIndex = target.rfind('.');

    std::string tc = (tcIndex == std::string::npos) ? "root" : target.substr(0, tcIndex);      // component name
    std::string cmd = (tcIndex == std::string::npos) ? target : target.substr(tcIndex + 1);    // parameter name
    std::string args = (splitIndex != std::string::npos ? buffer.substr(splitIndex + 1) : ""); // value

    const int numData = 10;
    var data[numData];

    data[0] = tc;
    data[1] = cmd;

    int index = 2;

    // Make a mutable copy of args since strtok modifies the string
    char *argsBuffer = strdup(args.c_str());
    char *pch = strtok(argsBuffer, ",");
    while (pch != NULL && index < numData)
    {
        std::string s(pch);

        bool isNumber = true;
        int numDotSigns = 0;
        for (size_t i = 0; i < s.length(); i++)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (std::isdigit(c))
                continue;

            if (c == '.')
            {
                numDotSigns++;
                if (numDotSigns > 1)
                {
                    isNumber = false;
                    break;
                }
                continue;
            }

            isNumber = false;
            break;
        }

        if (isNumber && !s.empty())
        {
            float f = std::strtof(s.c_str(), nullptr);
            int i = std::strtol(s.c_str(), nullptr, 10);

            if (f == i && s.find('.') == std::string::npos)
                data[index] = i;
            else
                data[index] = f;
        }
        else
        {
            data[index] = s;
        }

        pch = strtok(NULL, ",");
        index++;
    }
    free(argsBuffer);

    callback(data, index);
}

std::string StringHelpers::lowerCamelToTitleCase(std::string input)
{
    std::string result;
    bool wasSpecial = false;

    for (size_t i = 0; i < input.length(); i++)
    {
        char c = input[i];

        bool up = std::isupper(static_cast<unsigned char>(c));
        bool isNumber = std::isdigit(static_cast<unsigned char>(c));

        if (up || isNumber)
        {
            if (!wasSpecial && !result.empty())
                result += ' ';
            result += c;
        }
        else if (i == 0)
        {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        else
        {
            result += c;
        }

        wasSpecial = up || isNumber;
    }

    return result;
}

std::string StringHelpers::toLowerCase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return result;
}
std::string StringHelpers::toUpperCase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::toupper(c); });
    return result;
}

void StringHelpers::replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
void StringHelpers::macFromString(const std::string &mac, uint8_t *outMac)
{
    std::sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &outMac[0], &outMac[1], &outMac[2],
                &outMac[3], &outMac[4], &outMac[5]);
}

std::string StringHelpers::macToString(const uint8_t *mac)
{
    char buf[18];
    std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

std::string StringHelpers::byteToHexString(uint8_t byte)
{
    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02x", byte);
    return std::string(buf);
}

std::string StringHelpers::oscPathToSerial(const std::string &oscPath)
{
    std::string serialPath = oscPath;
    if (!serialPath.empty() && serialPath[0] == '/')
        serialPath.erase(0, 1);
    for (size_t i = 0; i < serialPath.size(); ++i)
        if (serialPath[i] == '/')
            serialPath[i] = '.';
    return serialPath;
}

std::string StringHelpers::serialPathToOSC(const std::string &serialPath)
{
    std::string oscPath = serialPath;
    for (size_t i = 0; i < oscPath.size(); ++i)
        if (oscPath[i] == '.')
            oscPath[i] = '/';
    if (oscPath.empty() || oscPath[0] != '/')
        oscPath = "/" + oscPath;
    return oscPath;
}