#pragma once

DeclareComponentSingleton(Script, "script", DMXListenerDerive)

    Script script;
DeclareStringParam(scriptAtLaunch, "");
DeclareIntParam(universe, 1);
DeclareIntParam(startChannel, 1);

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void sendScriptEvent(std::string eventName);
void sendScriptParamFeedback(std::string paramName, float value);

void onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) override;

virtual bool handleCommandInternal(const std::string &command, var *data, int numData);

DeclareComponentEventTypes(ScriptEvent, ScriptParamFeedback);
DeclareComponentEventNames("scriptEvent", "scriptParamFeedback");

EndDeclareComponent