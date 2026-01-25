#include "UnityIncludes.h"

ImplementSingleton(HWSerialComponent);

void HWSerialComponent::setupInternal(JsonObject o)
{
    AddBoolParamConfig(sendFeedback);

    bufferIndex = 0;
    memset(buffer, 0, 512);
    
    Serial.begin(115200);
}

void HWSerialComponent::updateInternal()
{
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n')
        {
            bufferIndex = 0;
            processMessage(std::string(buffer));
            memset(buffer, 0, 512);
        }
        else
        {
            if (bufferIndex < 512)
                buffer[bufferIndex] = c;
            bufferIndex++;
        }
    }
}

void HWSerialComponent::clearInternal()
{
}

void HWSerialComponent::processMessage(const std::string& buffer)
{
    RootComponent::instance->timeAtLastSignal = millis();
   
    if (buffer.substr(0, 2) == "yo")
    {
        const std::string msg = std::string("wassup ") + DeviceID.c_str() + " \"" + DeviceType.c_str() + "\" \"" + DeviceName.c_str() + "\" \"" + BLIP_VERSION + "\"";
        Serial.println(msg.c_str());
        return;
    }

    StringHelpers::processStringMessage(buffer, [this](var *data, int numdata)
                                        { sendEvent(MessageReceived, data, numdata); });

    // free(data);
}

void HWSerialComponent::sendMessage(const std::string& source, const std::string& command, var *data, int numData)
{
    if (!enabled) return;

    std::string msg = (source.empty() ? "" : source + ".") + command;
    for (int i = 0; i < numData; i++)
    {
        msg += ' ';
        msg += data[i].stringValue().c_str();
    }

    Serial.println(msg.c_str());
}

void HWSerialComponent::send(const std::string &message)
{
    Serial.println(message.c_str());
#ifdef USE_DISPLAY
#ifdef DISPLAY_SHOW_DBG
    RootComponent::instance->display.log(message.c_str());
#endif
#endif
}