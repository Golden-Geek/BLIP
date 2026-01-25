#pragma once

DeclareComponentSingleton(HWSerial, "serial", )

    char buffer[512];
byte bufferIndex;
DeclareBoolParam(sendFeedback, false);

void setupInternal(JsonObject o) override;
void updateInternal() override;
void clearInternal() override;

void processMessage(const std::string& buffer);
void sendMessage(const std::string& source, const std::string& command, var *data, int numData);

void send(const std::string &message);

DeclareComponentEventTypes(MessageReceived);
DeclareComponentEventNames("MessageReceived");


    EndDeclareComponent