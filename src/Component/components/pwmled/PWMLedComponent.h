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

#ifndef PWMLED_DEFAULT_USE_ALPHA
#define PWMLED_DEFAULT_USE_ALPHA false
#endif

#ifndef PWMLED_DEFAULT_RESOLUTION
#define PWMLED_DEFAULT_RESOLUTION 14
#endif

#ifndef PWMLED_DEFAULT_FREQUENCY
#define PWMLED_DEFAULT_FREQUENCY 1
#endif

#ifndef PWMLED_DEFAULT_16BIT_STREAM
#define PWMLED_DEFAULT_16BIT_STREAM false
#endif

#ifndef PWMLED_MAX_COUNT
#define PWMLED_MAX_COUNT 16
#endif

DeclareComponent(PWMLed, "pwmled", DMXListenerDerive)

    DeclareIntParam(rPin, PWMLED_DEFAULT_RPIN);
DeclareIntParam(gPin, PWMLED_DEFAULT_GPIN);
DeclareIntParam(bPin, PWMLED_DEFAULT_BPIN);
DeclareIntParam(wPin, PWMLED_DEFAULT_WPIN);
DeclareIntParam(pwmResolution, PWMLED_DEFAULT_RESOLUTION);
DeclareIntParam(pwmFrequency, PWMLED_DEFAULT_FREQUENCY);
DeclareBoolParam(useAlpha, PWMLED_DEFAULT_USE_ALPHA);
DeclareBoolParam(rgbwMode, false);
DeclareColorParam(color, 0, 0, 0, 1);

#ifdef PWMLED_USE_STREAMING
DeclareIntParam(universe, 0);
DeclareIntParam(ledIndex, 0);
DeclareBoolParam(use16bit, PWMLED_DEFAULT_16BIT_STREAM);
#endif

int pwmChannels[4];
int attachedPins[4]{-1, -1, -1, -1};

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

void onLedStreamReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) override;

    EndDeclareComponent;

// Manager

DeclareComponentManager(PWMLed, PWMLED, pwmLed, pwmLed, PWMLED_MAX_COUNT)

EndDeclareComponent