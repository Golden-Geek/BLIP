#include "UnityIncludes.h"

void StringHelpers::processStringMessage(const String &buffer, std::function<void(var *data, int numData)> callback)
{
    int splitIndex = buffer.indexOf(' ');

    String target = buffer.substring(0, splitIndex); // target component

    int tcIndex = target.lastIndexOf('.');

    String tc = tcIndex == -1 ? "root" : target.substring(0, tcIndex);        // component name
    String cmd = target.substring(tcIndex + 1);                               // parameter name
    String args = (splitIndex != -1 ? buffer.substring(splitIndex + 1) : ""); // value

    const int numData = 10;
    var data[numData];

    data[0] = tc;
    data[1] = cmd;

    int index = 2;
    // COUNT

    // Make a mutable copy of args since strtok modifies the string
    String argsCopy = args;
    char *argsBuffer = strdup(argsCopy.c_str());
    char *pch = strtok(argsBuffer, ",");
    while (pch != NULL && index < numData)
    {
        String s = String(pch);

        bool isNumber = true;
        for (byte i = 0; i < s.length(); i++)
        {
            char c = s.charAt(i);
            if (c != '.' && c != '-' && c != '+' && !isDigit(c))
            {
                isNumber = false;
                break;
            }
        }

        if (isNumber)
        {
            float f = s.toFloat();
            int i = s.toInt();

            if (f == i && s.indexOf('.') == -1)
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
String StringHelpers::lowerCamelToTitleCase(String input)
{
    String result = "";
    bool wasSpecial = false;

    for (int i = 0; i < input.length(); i++)
    {
        char c = input.charAt(i);
        String s = input.substring(i, i + 1);

        bool up = isUpperCase(c);
        bool isNumber = isDigit(c);

        if (up || isNumber)
        {
            if (!wasSpecial)
                result += " ";
            result += c;
        }
        else if (i == 0)
        {
            result += (char)toUpperCase(c);
        }
        else
        {
            result += c;
        }

        wasSpecial = up || isNumber;
    }

    return result;
}

String StringHelpers::ipToString(IPAddress ip)
{
    return String(ip[0]) +
           "." + String(ip[1]) +
           "." + String(ip[2]) +
           "." + String(ip[3]);
}

void StringHelpers::macFromString(const String &mac, uint8_t *outMac)
{
    sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &outMac[0], &outMac[1], &outMac[2],
           &outMac[3], &outMac[4], &outMac[5]);
}


String StringHelpers::macToString(const uint8_t *mac)
{
    return String(mac[0], HEX) + ":" +
           String(mac[1], HEX) + ":" +
           String(mac[2], HEX) + ":" +
           String(mac[3], HEX) + ":" +
           String(mac[4], HEX) + ":" +
           String(mac[5], HEX);
}