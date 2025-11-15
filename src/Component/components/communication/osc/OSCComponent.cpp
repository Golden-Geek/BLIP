#include "UnityIncludes.h"

ImplementSingleton(OSCComponent);

void OSCComponent::setupInternal(JsonObject o)
{
    udpIsInit = false;
    mdnsIsInit = false;

    AddStringParamConfig(remoteHost);
    AddBoolParamConfig(sendFeedback);
    AddBoolParamConfig(isAlive);
}

void OSCComponent::updateInternal()
{
    receiveOSC();
}

void OSCComponent::clearInternal()
{

    if (mdnsIsInit)
    {
        DBG("MDNS Remove service");
        esp_err_t err = mdns_service_remove_all();

        if (err == ESP_OK)
        {
            DBG("Successfully removed all services.");
        }
        else
        {
            DBG(String("Failed to remove all services: ") + String(esp_err_to_name(err)));
            // Handle error
        }
        MDNS.end();
    }
}

void OSCComponent::onEnabledChanged()
{
    setupConnection();
}

void OSCComponent::setupConnection()
{
    bool shouldConnect = enabled && WifiComponent::instance->state == WifiComponent::Connected;

    mdnsIsInit = false;
    udpIsInit = false;
    if (shouldConnect)
    {
        NDBG("Start OSC Receiver on " + String(OSC_LOCAL_PORT));
        udp.begin(OSC_LOCAL_PORT);
        udp.clear();
        SetParam(isAlive, true);

        if (MDNS.begin((DeviceName).c_str()))
        {
            MDNS.setInstanceName("BLIP - " + DeviceName);
            NDBG("OSC Zeroconf started");
            MDNS.addService("osc", "udp", 9000);
            MDNS.addServiceTxt("osc", "udp", "deviceID", DeviceID.c_str());
            MDNS.addServiceTxt("osc", "udp", "deviceName", DeviceName.c_str());
            MDNS.addServiceTxt("osc", "udp", "deviceType", DeviceType.c_str());

            MDNS.addService("oscjson", "tcp", 80);
            MDNS.addServiceTxt("oscjson", "tcp", "deviceID", DeviceID.c_str());
            MDNS.addServiceTxt("oscjson", "tcp", "deviceName", DeviceName.c_str());
            MDNS.addServiceTxt("oscjson", "tcp", "deviceType", DeviceType.c_str());
            mdnsIsInit = true;
        }
        else
        {
            NDBG("Error setting up MDNS responder!");
        }

        udpIsInit = true;
    }
    else
    {
        // NDBG("Stopping Receiver");
        udp.clear();
        udp.stop();
        SetParam(isAlive, false);
        MDNS.end();
        udpIsInit = false;
    }
}

void OSCComponent::receiveOSC()
{
    if (!udpIsInit)
        return;

    OSCMessage msg;
    int size;
    if ((size = udp.parsePacket()) > 0)
    {
        // DBG("Received OSC message");
        while (size--)
            msg.fill(udp.read());
        if (!msg.hasError())
            processMessage(msg);
        else
        {
            NDBG("Error parsing OSC message");
        }
    }
}

void OSCComponent::processMessage(OSCMessage &msg)
{
    if (msg.match("/yo"))
    {
        char hostData[32];
        msg.getString(0, hostData, 32);

        NDBG("Yo received from : " + String(hostData));

        SetParam(remoteHost, hostData);

        OSCMessage msg("/wassup");

        msg.add(WifiComponent::instance->getIP().c_str());
        msg.add(DeviceID.c_str());
        msg.add(DeviceType.c_str());
        msg.add(DeviceName.c_str());
        msg.add(BLIP_VERSION);

        sendMessage(msg);
    }
    else if (msg.match("/ping"))
    {
        // NDBG("Received ping");
        SetParam(isAlive, true);

        if (msg.size() > 0)
        {
            char hostData[32];
            msg.getString(0, hostData, 32);
            SetParam(remoteHost, hostData);
        }

        OSCMessage msg("/pong");
        msg.add(DeviceID.c_str());
        sendMessage(msg);
    }
    else
    {
        char addr[64];
        msg.getAddress(addr);
        String addrStr = String(addr).substring(1);
        addrStr.replace('/', '.');
        int tcIndex = addrStr.lastIndexOf('.');
        String tc = tcIndex == -1 ? "root" : addrStr.substring(0, tcIndex); // component name
        String cmd = addrStr.substring(tcIndex + 1);

        const int numData = 10; // max 10-2 = 8 arguments
        var data[numData];
        data[0] = tc;
        data[1] = cmd;
        // NDBG(data[0].stringValue() + "." + data[1].stringValue());

        for (int i = 0; i < msg.size() && i < numData - 2; i++)
        {
            data[i + 2] = OSCArgumentToVar(msg, i);
        }

        sendEvent(MessageReceived, data, msg.size() + 2);
    }
}

void OSCComponent::sendMessage(OSCMessage &msg)
{
    NDBG("Sending OSC message to " + remoteHost + " : " + String(msg.getAddress()));
    
    if (!udpIsInit || !enabled || remoteHost.length() == 0)
        return;

    if (WifiComponent::instance->state != WifiComponent::Connected)
        return;

    char addr[32];
    msg.getAddress(addr);
    udp.beginPacket((char *)remoteHost.c_str(), OSC_REMOTE_PORT);
    msg.send(udp);
    udp.endPacket();
    msg.empty();
}

void OSCComponent::sendMessage(String address)
{
    if (!udpIsInit || !enabled || remoteHost.length() == 0)
        return;

    if (WifiComponent::instance->state != WifiComponent::Connected)
        return;

    OSCMessage m(address.c_str());
    sendMessage(m);
}

void OSCComponent::sendMessage(const String &source, const String &command, var *data, int numData)
{
    if (!udpIsInit || !enabled || remoteHost.length() == 0)
        return;

    if (WifiComponent::instance->state != WifiComponent::Connected)
        return;

    OSCMessage msg = createMessage(source, command, data, numData);
    sendMessage(msg);
}

OSCMessage OSCComponent::createMessage(const String &source, const String &command, const var *data, int numData, bool addID)
{
    OSCMessage msg((source + "/" + command).c_str());
    if (addID)
        msg.add(DeviceID.c_str());
    for (int i = 0; i < numData; i++)
    {
        switch (data[i].type)
        {
        case 'f':
            msg.add(data[i].floatValue());
            break;

        case 'i':
            msg.add(data[i].intValue());
            break;

        case 's':
            msg.add(data[i].stringValue().c_str());
            break;

        case 'b':
            msg.add(data[i].boolValue());
            break;

        case 'r':
        {
            uint32_t col = (uint32_t)data[i].intValue();
            oscrgba_t rgba = {(uint8_t)(col >> 24), (uint8_t)((col >> 16) & 0xFF), (uint8_t)((col >> 8) & 0xFF), (uint8_t)(col & 0xFF)};
            msg.add(rgba);
        }
        break;

        case 'T':
            msg.add(true);
            break;

        case 'F':
            msg.add(false);
            break;
        }
    }

    return msg;
}


OSCMessage OSCComponent::createMessage(const var* data, int numData, bool addID)
{
    if(numData < 2)
        return OSCMessage();

    
    String source = StringHelpers::serialPathToOSC(data[0].stringValue());
    String command = data[1].stringValue();
    return createMessage(source, command, data + 2, numData - 2, addID);
}

var OSCComponent::OSCArgumentToVar(OSCMessage &m, int index)
{
    if (m.isString(index))
    {
        char str[32];
        m.getString(index, str);
        return str;
    }
    else if (m.isInt(index))
        return (int)m.getInt(index);
    else if (m.isFloat(index))
        return m.getFloat(index);
    else if (m.isBoolean(index))
        return m.getBoolean(index);
    else if (m.isRgba(index))
    {
        oscrgba_t rgba = m.getRgba(index);
        // DBG(String(rgba.r) + " " + String(rgba.g) + " " + String(rgba.b) + " " + String(rgba.a));
        int col = (rgba.a << 24) | (rgba.b << 16) | (rgba.g << 8) | rgba.r;
        return col;
    }
    else if (m.isMidi(index))
    {
        oscmidi_t midi = m.getMidi(index);
        return midi.status << 24 | midi.channel << 16 | midi.data1 << 8 | midi.data2;
    }
    else if (m.isTime(index))
    {
        osctime_t time = m.getTime(index);
        float absTime = time.seconds + time.fractionofseconds / 1000000.0;
        return absTime;
    }

    NDBG("OSC Type not supported !");
    return var(0);
}
