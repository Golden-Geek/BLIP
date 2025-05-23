#pragma once

#define OSC_LOCAL_PORT 9000
#define OSC_REMOTE_PORT 10000
#define OSC_PING_TIMEOUT 5000

DeclareComponentSingleton(OSC, "osc", )

    WiFiUDP udp;
bool udpIsInit;
DeclareStringParam(remoteHost, "");
DeclareBoolParam(isAlive, "");
DeclareBoolParam(sendFeedback, false);

void setupInternal(JsonObject o) override;
void updateInternal() override;
void clearInternal() override;

void onEnabledChanged() override;

void setupConnection();

void receiveOSC();
void processMessage(OSCMessage &m);

void saveRemoteHost(String ip);

void sendMessage(OSCMessage &m);
void sendMessage(String address);
void sendMessage(const String &source, const String &command, var *data, int numData);

static OSCMessage createMessage(const String &source, const String &command, const var *data, int numData, bool addID);

// Helper
var OSCArgumentToVar(OSCMessage &m, int index);

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");

    HandleSetParamInternalStart
        CheckAndSetParam(remoteHost);
        CheckAndSetParam(sendFeedback);
    HandleSetParamInternalEnd;

    FillSettingsInternalStart
        FillSettingsParam(remoteHost);
        FillSettingsParam(sendFeedback);
    FillSettingsInternalEnd;

    FillOSCQueryInternalStart
        FillOSCQueryStringParam(remoteHost);
    FillOSCQueryBoolParam(sendFeedback);
    FillOSCQueryBoolParamReadOnly(isAlive);
    FillOSCQueryInternalEnd

EndDeclareComponent