#pragma once

DeclareComponentSingleton(Script, "script", )

    Script script;
DeclareIntParam(updateRate, 50); // in Hz
DeclareStringParam(scriptAtLaunch, "");

long lastUpdateTime = 0;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void sendScriptEvent(String eventName);
void sendScriptParamFeedback(String paramName, float value);

virtual bool handleCommandInternal(const String &command, var *data, int numData);

DeclareComponentEventTypes(ScriptEvent, ScriptParamFeedback);
DeclareComponentEventNames("scriptEvent", "scriptParamFeedback");

HandleSetParamInternalStart
    CheckAndSetParam(updateRate);
    CheckAndSetParam(scriptAtLaunch);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(updateRate);
    FillSettingsParam(scriptAtLaunch);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryIntParam(updateRate);
    FillOSCQueryStringParam(scriptAtLaunch);
FillOSCQueryInternalEnd;

EndDeclareComponent