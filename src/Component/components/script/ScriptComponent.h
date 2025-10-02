#pragma once

DeclareComponentSingleton(Script, "script", )

    Script script;
DeclareStringParam(scriptAtLaunch, "");

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

virtual bool handleCommandInternal(const String &command, var *data, int numData);

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