#pragma once

#ifndef PWMLED_DEFAULT_RPIN
#define PWMLED_DEFAULT_RPIN -1
#endif

#ifndef PWMLED_DEFAULT_GPIN
#define PWMLED_DEFAULT_GPIN -1
#endif

#ifndef PWMLED_DEFAULT_BPIN
#define PWMLED_DEFAULT_BPIN -1
#endif

#ifndef PWMLED_DEFAULT_WPIN
#define PWMLED_DEFAULT_WPIN -1
#endif

DeclareComponent(PWMLed, "pwmled", LedStreamListenerDerive )

    DeclareIntParam(rPin, PWMLED_DEFAULT_RPIN);
DeclareIntParam(gPin, PWMLED_DEFAULT_GPIN);
DeclareIntParam(bPin, PWMLED_DEFAULT_BPIN);
DeclareIntParam(wPin, PWMLED_DEFAULT_WPIN);
// DeclareIntParam(whiteTemperature, 4500);
DeclareBoolParam(useAlpha, true);
DeclareBoolParam(rgbwMode, false);
DeclareColorParam(color, 0, 0, 0, 1);

#ifdef PWMLED_USE_STREAMING
DeclareIntParam(universe, 0);
DeclareIntParam(startChannel, 1);
#endif

int pwmChannels[4];

#ifdef USE_WIFI
WifiComponent::ConnectionState prevState;
#ifdef USE_ESPNOW
bool prevPairingMode = false;
#else
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

void onLedStreamReceived(uint16_t universe, const uint8_t *data, uint16_t len) override;

HandleSetParamInternalStart
    CheckAndSetParam(rPin);
CheckAndSetParam(gPin);
CheckAndSetParam(bPin);
CheckAndSetParam(wPin);
// CheckAndSetParam(whiteTemperature);
CheckAndSetParam(useAlpha);
CheckAndSetParam(rgbwMode);
CheckAndSetParam(color);
#ifdef PWMLED_USE_STREAMING
CheckAndSetParam(universe);
CheckAndSetParam(startChannel);
#endif
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(rPin);
FillSettingsParam(gPin);
FillSettingsParam(bPin);
FillSettingsParam(wPin);
// FillSettingsParam(whiteTemperature);
FillSettingsParam(useAlpha);
FillSettingsParam(rgbwMode);
#ifdef PWMLED_USE_STREAMING
FillSettingsParam(universe);
FillSettingsParam(startChannel);
#endif
FillSettingsInternalEnd

    FillOSCQueryInternalStart
        FillOSCQueryIntParam(rPin);
FillOSCQueryIntParam(gPin);
FillOSCQueryIntParam(bPin);
FillOSCQueryIntParam(wPin);
// FillOSCQueryIntParam(whiteTemperature);
FillOSCQueryBoolParam(useAlpha);
FillOSCQueryBoolParam(rgbwMode);
FillOSCQueryColorParam(color);
#ifdef PWMLED_USE_STREAMING
FillOSCQueryIntParam(universe);
FillOSCQueryIntParam(startChannel);
#endif
FillOSCQueryInternalEnd

    EndDeclareComponent;

// Manager

DeclareComponentManager(PWMLed, PWMLED, pwmLed, pwmLed)

    void addItemInternal(int index)
{
}
EndDeclareComponent