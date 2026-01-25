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

void sendParamFeedback(Component *c, const ParamInfo* paramInfo);
void sendMessage(Component *c, const std::string &mName, const std::string &val);
void sendEventFeedback(const ComponentEvent &e);
void sendDebug(const std::string& msg, const std::string&source = "", const std::string& type = "info");

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

EndDeclareComponent