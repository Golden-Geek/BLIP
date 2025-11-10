#pragma once

DeclareComponentSingleton(Script, "script", )

    Script script;
DeclareStringParam(scriptAtLaunch, "");

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
    CheckAndSetParam(scriptAtLaunch);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(scriptAtLaunch);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryStringParam(scriptAtLaunch);
FillOSCQueryInternalEnd;

EndDeclareComponent