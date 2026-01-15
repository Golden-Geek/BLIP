#pragma once

#if defined USE_DMX || defined USE_ARTNET
#define DMXListenerActualDerive DMXListenerDerive
#else
#define DMXListenerActualDerive
#endif

DeclareComponentSingleton(Script, "script", DMXListenerActualDerive)

    Script script;
DeclareIntParam(updateRate, 50); // in Hz
DeclareStringParam(scriptAtLaunch, "");
DeclareIntParam(universe, 1);
DeclareIntParam(startChannel, 1);

long lastUpdateTime = 0;

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
    CheckAndSetParam(updateRate);
CheckAndSetParam(scriptAtLaunch);
CheckAndSetParam(universe);
CheckAndSetParam(startChannel);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(updateRate);
FillSettingsParam(scriptAtLaunch);
FillSettingsParam(universe);
FillSettingsParam(startChannel);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryIntParam(updateRate);
FillOSCQueryStringParam(scriptAtLaunch);
FillOSCQueryIntParam(universe);
FillOSCQueryIntParam(startChannel);
FillOSCQueryInternalEnd;

EndDeclareComponent