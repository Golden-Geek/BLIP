#pragma once

#if defined USE_DMX || defined USE_ARTNET
#define DMXListenerActualDerive DMXListenerDerive
#else
#define DMXListenerActualDerive
#endif

DeclareComponentSingleton(Script, "script", DMXListenerActualDerive)

    Script script;
DeclareStringParam(scriptAtLaunch, "");
DeclareIntParam(universe, 1);
DeclareIntParam(startChannel, 1);

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void sendScriptEvent(String eventName);
void sendScriptParamFeedback(String paramName, float value);

void onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) override;

virtual bool handleCommandInternal(const String &command, var *data, int numData);

DeclareComponentEventTypes(ScriptEvent, ScriptParamFeedback);
DeclareComponentEventNames("scriptEvent", "scriptParamFeedback");

HandleSetParamInternalStart
CheckAndSetParam(scriptAtLaunch);
CheckAndSetParam(universe);
CheckAndSetParam(startChannel);
HandleSetParamInternalEnd;

FillSettingsInternalStart
FillSettingsParam(scriptAtLaunch);
FillSettingsParam(universe);
FillSettingsParam(startChannel);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
FillOSCQueryStringParam(scriptAtLaunch);
FillOSCQueryIntParam(universe);
FillOSCQueryIntParam(startChannel);
FillOSCQueryInternalEnd;

EndDeclareComponent