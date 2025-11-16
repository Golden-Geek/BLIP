#pragma once
DeclareComponentSingleton(Communication, "comm", )

#ifdef USE_SERIAL
    HWSerialComponent serial;
#endif

#ifdef USE_OSC
OSCComponent osc;
#endif

#ifdef USE_ESPNOW
ESPNowComponent espNow;
#endif

#ifdef USE_SERVER
WebServerComponent server;
#endif

void setupInternal(JsonObject o) override;

void onChildComponentEvent(const ComponentEvent &e) override;

void sendParamFeedback(Component *c, void *param, const String &pName, Component::ParamType pType);
void sendMessage(Component *c, const String &mName, const String &val);
void sendEventFeedback(const ComponentEvent &e);

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

EndDeclareComponent