#include "UnityIncludes.h"
#include "CommunicationComponent.h"

ImplementSingleton(CommunicationComponent);

void CommunicationComponent::setupInternal(JsonObject o)
{
#ifdef USE_SERIAL
    AddOwnedComponent(&serial);
#endif

#ifdef USE_OSC
    AddOwnedComponent(&osc);
#endif

#ifdef USE_ESPNOW
    AddOwnedComponent(&espNow);
#endif

#ifdef USE_SERVER
    AddOwnedComponent(&server);
#endif
}

void CommunicationComponent::onChildComponentEvent(const ComponentEvent &e)
{
#ifdef USE_SERIAL
    if (e.component == &serial && e.type == HWSerialComponent::MessageReceived)
    {

        sendEvent(MessageReceived, e.data, e.numData);

#ifdef USE_ESPNOW
#ifdef ESPNOW_BRIDGE
        espNow.routeMessage(e.data, e.numData);
#endif
#endif
    }
#endif

#ifdef USE_OSC
    if (e.component == &osc && e.type == OSCComponent::MessageReceived)
    {
        sendEvent(MessageReceived, e.data, e.numData);

#ifdef USE_ESPNOW
#ifdef ESPNOW_BRIDGE
        espNow.routeMessage(e.data, e.numData);
#endif
#endif
    }
#endif

#ifdef USE_ESPNOW
    if (e.component == &espNow)
    {
        if (e.type == ESPNowComponent::MessageReceived)
        {
#ifdef ESPNOW_BRIDGE
            if (e.data[0].stringValue().starts_with("dev.")) // message from device, to route
            {
#ifdef USE_OSC
                if (osc.sendFeedback)
                {
                    OSCMessage msg = OSCComponent::createMessage(e.data, e.numData, false);
                    osc.sendMessage(msg);
                }
#endif

#ifdef USE_SERIAL
                if (serial.sendFeedback)
                    serial.sendMessage(e.data[0].stringValue(), e.data[1].stringValue(), &e.data[2], e.numData - 2);
#endif

#ifdef USE_SERVER
                if (server.sendFeedback)
                {

                    server.sendParamFeedback(StringHelpers::serialPathToOSC(e.data[0].stringValue()), e.data[1].stringValue(), &e.data[2], e.numData - 2);
                }
#endif
            }

#else
            sendEvent(MessageReceived, e.data, e.numData);
#endif
        }
    }
#endif
}

void CommunicationComponent::sendParamFeedback(Component *c, const ParamInfo *paramInfo)
{
    void *param = paramInfo->ptr;
    ParamType pType = paramInfo->type;
    std::string pName = paramInfo->name;
    int numData = 1;
    var data[4];

    switch (pType)
    {

    case ParamType::Bool:
        data[0] = (*(bool *)param);
        break;

    case ParamType::Int:
        data[0] = (*(int *)param);
        break;

    case ParamType::TypeEnum:
    {
        const std::string *options = paramInfo->enumOptions;
        int index = (*(int *)param);
        if (options && index >= 0)
            data[0] = options[index];
        else
            data[0] = "";
    }
    break;

    case ParamType::Float:
        data[0] = (*(float *)param);
        break;

    case ParamType::Str:
        data[0] = (*(std::string *)param);
        break;

    case ParamType::P2D:
    case ParamType::P3D:
    {
        numData = pType == ParamType::P2D ? 2 : 3;
        float *vals = (float *)param;
        data[0] = vals[0];
        data[1] = vals[1];
        if (pType == ParamType::P3D)
            data[2] = vals[2];
    }
    break;

    case ParamType::TypeColor:
    {
        numData = 4;
        data[0] = ((float *)param)[0];
        data[1] = ((float *)param)[1];
        data[2] = ((float *)param)[2];
        data[3] = ((float *)param)[3];
    }
    break;

    default:
        break;
    }

#ifdef USE_SERIAL
    if (serial.sendFeedback)
    {
        std::string baseAddress = c->getFullPath(false, false, true);
        serial.sendMessage(baseAddress, pName, data, numData);
    }
#endif

#if defined USE_OSC || defined USE_ESPNOW
    std::string baseAddress = c->getFullPath();
#endif

#ifdef USE_OSC
    if (osc.sendFeedback)
    {
        osc.sendMessage(baseAddress, pName, data, numData);
    }
#endif

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
    if (espNow.sendFeedback)
        espNow.sendMessage(-1, baseAddress, pName, data, numData);
#endif
#endif

#ifdef USE_SERVER
    // maybe should find away so calls are not crossed with websocket ?
    if (server.sendFeedback)
        server.sendParamFeedback(c, pName, data, numData);
#endif
}

void CommunicationComponent::sendEventFeedback(const ComponentEvent &e)
{
#ifdef USE_SERIAL
    if (serial.sendFeedback)
    {
        std::string serialAddress = e.component->getFullPath(false, false, true);
        serial.sendMessage(serialAddress, e.getName(), e.data, e.numData);
    }
#endif

#if defined USE_OSC || defined USE_ESPNOW
    std::string baseAddress = e.component->getFullPath();
#endif

#ifdef USE_OSC
    if (osc.sendFeedback)
    {
        osc.sendMessage(baseAddress, e.getName(), e.data, e.numData);
    }
#endif

#ifdef USE_SERVER
    if (server.sendFeedback)
    {
        server.sendParamFeedback(baseAddress, e.getName(), e.data, e.numData);
    }
#endif

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
    if (espNow.sendFeedback)
    {
        espNow.sendMessage(-1, baseAddress, e.getName(), e.data, e.numData);
    }
#endif
#endif
}

void CommunicationComponent::sendMessage(Component *c, const std::string &mName, const std::string &val)
{
    var data[1]{val};

#ifdef USE_SERIAL
    std::string serialAddress = c->getFullPath(false, false, true);
    serial.sendMessage(serialAddress, mName, data, 1);
#endif

#ifdef USE_OSC
    std::string oscAddress = c->getFullPath();
    osc.sendMessage(oscAddress, mName, data, 1);
#endif

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
    std::string espnowAddress = c->getFullPath(false, false, true);
    espNow.sendMessage(-1, espnowAddress, mName, data, 1);
#endif
#endif
}

void CommunicationComponent::sendDebug(const std::string &msg, const std::string &source, const std::string &type)
{
#ifdef USE_SERIAL
    serial.send("[" + source + "] " + msg);
#endif

#ifdef USE_SERVER
    if (server.sendDebugLogs)
    {
        server.sendDebugLog(msg, source, type);
    }
#endif
}