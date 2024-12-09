#include "UnityIncludes.h"

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
}

void CommunicationComponent::onChildComponentEvent(const ComponentEvent &e)
{
#ifdef USE_SERIAL
    if (e.component == &serial && e.type == SerialComponent::MessageReceived)
    {
        sendEvent(MessageReceived, e.data, e.numData);
#ifdef ESPNOW_BRIDGE
        espNow.sendMessage("", e.data[0].stringValue(), e.data[1].stringValue(), &e.data[2], e.numData - 2);
#endif
    }
#endif

#ifdef USE_OSC
    if (e.component == &osc && e.type == OSCComponent::MessageReceived)
    {
        sendEvent(MessageReceived, e.data, e.numData);
    }

#ifdef ESPNOW_BRIDGE
    espNow.sendMessage("", e.data[0].stringValue(), e.data[1].stringValue(), &e.data[2], e.numData - 2);
#endif
#endif

#ifdef USE_ESPNOW
    if (e.component == &espNow)
    {
        if (e.type == ESPNowComponent::MessageReceived)
            sendEvent(MessageReceived, e.data, e.numData);
    }

#endif
}

void CommunicationComponent::sendParamFeedback(Component *c, void *param, const String &pName, Component::ParamType pType)
{
    int numData = 1;
    var data[3];
    switch (pType)
    {
    case Component::ParamType::Trigger:
        break;

    case Component::ParamType::Bool:
        data[0] = (*(bool *)param);
        break;

    case Component::ParamType::Int:
        data[0] = (*(int *)param);
        break;

    case Component::ParamType::Float:
        data[0] = (*(float *)param);
        break;

    case Component::ParamType::Str:
        data[0] = (*(String *)param);
        break;

    case Component::ParamType::P2D:
    case Component::ParamType::P3D:
    {
        numData = pType == Component::ParamType::P2D ? 2 : 3;
        float *vals = (float *)param;
        data[0] = vals[0];
        data[1] = vals[1];
        if (pType == Component::ParamType::P3D)
            data[2] = vals[2];
    }
    break;

    default:
        break;
    }

    String baseAddress = c->getFullPath();

#ifdef USE_SERIAL
    if (serial.sendFeedback)
        serial.sendMessage(baseAddress, pName, data, numData);
#endif

#ifdef USE_OSC
    if (osc.sendFeedback)
        osc.sendMessage(baseAddress, pName, data, numData);
#endif

#ifdef USE_SERVER
    // maybe should find away so calls are not crossed with websocket ?
    if (WebServerComponent::instance->sendFeedback)
        WebServerComponent::instance->sendParamFeedback(c, pName, data, numData);
#endif
}

void CommunicationComponent::sendEventFeedback(const ComponentEvent &e)
{
    String baseAddress = e.component->getFullPath();
#ifdef USE_SERIAL
    if (serial.sendFeedback)
        serial.sendMessage(baseAddress, e.getName(), e.data, e.numData);
#endif

#ifdef USE_OSC
    if (osc.sendFeedback)
        osc.sendMessage(baseAddress, e.getName(), e.data, e.numData);
#endif
}

void CommunicationComponent::sendMessage(Component *c, const String &mName, const String &val)
{
    String baseAddress = c->getFullPath();

    var data[1]{val};
#ifdef USE_SERIAL
    serial.sendMessage(baseAddress, mName, data, 1);
#endif

#ifdef USE_OSC
    osc.sendMessage(baseAddress, mName, data, 1);
#endif
}
