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
            processMessage(buffer);
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

void HWSerialComponent::processMessage(String buffer)
{
    if (buffer.substring(0, 2) == "yo")
    {
        Serial.println("wassup " + DeviceID + " \"" + String(DeviceType) + "\" \"" + String(DeviceName) + "\" \"" + String(BLIP_VERSION) + "\"");
        return;
    }

    StringHelpers::processStringMessage(buffer, [this](var *data, int numdata)
                                        { sendEvent(MessageReceived, data, numdata); });

    // free(data);
}

void HWSerialComponent::sendMessage(String source, String command, var *data, int numData)
{
    String msg = (source == "" ? "" : source + ".") + command;
    for (int i = 0; i < numData; i++)
    {
        msg += " " + data[i].stringValue();
    }

    Serial.println(msg);
}

void HWSerialComponent::send(const String &message)
{
    Serial.println(message);
#ifdef USE_DISPLAY
#ifdef DISPLAY_SHOW_DBG
    RootComponent::instance->display.log(message);
#endif
#endif
}