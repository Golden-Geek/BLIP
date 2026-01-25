#pragma once

#define OSC_LOCAL_PORT 9000
#define OSC_REMOTE_PORT 10000
#define OSC_PING_TIMEOUT 5000

DeclareComponentSingleton(OSC, "osc", )

    WiFiUDP udp;
bool udpIsInit;
bool mdnsIsInit;
DeclareStringParam(remoteHost, "");
DeclareIntParam(remotePort, OSC_REMOTE_PORT);
DeclareBoolParam(isAlive, "");
DeclareBoolParam(sendFeedback, false);

void setupInternal(JsonObject o) override;
void updateInternal() override;
void clearInternal() override;

void onEnabledChanged() override;

void setupConnection();

void receiveOSC();
void processMessage(OSCMessage &m);

void saveRemoteHost(std::string ip);

void sendMessage(OSCMessage &m);
void sendMessage(std::string address);
void sendMessage(const std::string &source, const std::string &command, var *data, int numData);

static OSCMessage createMessage(const std::string &source, const std::string &command, const var *data, int numData, bool addID = true);
static OSCMessage createMessage(const var* data, int numData, bool addID = true);

// Helper
var OSCArgumentToVar(OSCMessage &m, int index);

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");


EndDeclareComponent