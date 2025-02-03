#pragma once

DeclareComponent(PWMLed, "pwmled", )

    DeclareIntParam(rPin, -1);
DeclareIntParam(gPin, -1);
DeclareIntParam(bPin, -1);
DeclareIntParam(wPin, -1);
DeclareIntParam(whiteTemperature, 4500);
DeclareBoolParam(useAlpha, true);
DeclareColorParam(color, 0, 0, 0, 1);

int pwmChannels[4];

#ifdef USE_WIFI
#ifdef USE_ESPNOW
bool prevPairingMode = false;
#else
WifiComponent::ConnectionState prevState;
#endif
#endif

virtual void setupInternal(JsonObject o) override;
virtual bool initInternal() override;
virtual void updateInternal() override;
virtual void clearInternal() override;

virtual void setupPins();
void updatePins();

void setColor(float r, float g, float b, float a = 1, bool show = true);

void RGBToRGBW(float r, float g, float b, float &rOut, float &gOut, float &bOut, float &wOut);

void paramValueChangedInternal(void *param) override;

HandleSetParamInternalStart
    CheckAndSetParam(rPin);
CheckAndSetParam(gPin);
CheckAndSetParam(bPin);
CheckAndSetParam(wPin);
CheckAndSetParam(whiteTemperature);
CheckAndSetParam(useAlpha);
CheckAndSetParam(color);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(rPin);
FillSettingsParam(gPin);
FillSettingsParam(bPin);
FillSettingsParam(wPin);
FillSettingsParam(whiteTemperature);
FillSettingsParam(useAlpha);
FillSettingsInternalEnd

    FillOSCQueryInternalStart
        FillOSCQueryIntParam(rPin);
FillOSCQueryIntParam(gPin);
FillOSCQueryIntParam(bPin);
FillOSCQueryIntParam(wPin);
FillOSCQueryIntParam(whiteTemperature);
FillOSCQueryBoolParam(useAlpha);
FillOSCQueryColorParam(color);
FillOSCQueryInternalEnd

    EndDeclareComponent;

// Manager

DeclareComponentManager(PWMLed, PWMLED, pwmLed, pwmLed)

    void addItemInternal(int index)
{
}
EndDeclareComponent