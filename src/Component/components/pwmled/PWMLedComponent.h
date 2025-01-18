#pragma once

DeclareComponent(PWMLed, "pwmled", )

    DeclareIntParam(rPin, -1);
DeclareIntParam(gPin, -1);
DeclareIntParam(bPin, -1);
DeclareIntParam(wPin, -1);
DeclareColorParam(color, 0, 0, 0, 1);

int pwmChannels[4];

virtual void setupInternal(JsonObject o) override;
virtual bool initInternal() override;
virtual void updateInternal() override;
virtual void clearInternal() override;

virtual void setupPins();
void updatePins();

    void paramValueChangedInternal(void *param) override;

HandleSetParamInternalStart
    CheckAndSetParam(rPin);
CheckAndSetParam(gPin);
CheckAndSetParam(bPin);
CheckAndSetParam(wPin);
CheckAndSetParam(color);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(rPin);
FillSettingsParam(gPin);
FillSettingsParam(bPin);
FillSettingsParam(wPin);
FillSettingsParam4(color);
FillSettingsInternalEnd

    FillOSCQueryInternalStart
        FillOSCQueryIntParam(rPin);
FillOSCQueryIntParam(gPin);
FillOSCQueryIntParam(bPin);
FillOSCQueryIntParam(wPin);
FillOSCQueryColorParam(color);
FillOSCQueryInternalEnd

    EndDeclareComponent;

// Manager

DeclareComponentManager(PWMLed, PWMLED, pwmLed, pwmLed)

    void addItemInternal(int index)
{
}
EndDeclareComponent