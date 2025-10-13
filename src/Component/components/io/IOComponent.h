#pragma once

#ifndef IO_MAX_COUNT
#define IO_MAX_COUNT 4
#endif

#ifndef IO_DEFAULT_PIN
#define IO_DEFAULT_PIN -1
#endif

#ifndef IO_DEFAULT_MODE
#define IO_DEFAULT_MODE IOComponent::D_OUTPUT
#endif

DeclareComponent(IO, "io", )

    enum PinMode { D_INPUT,
                   D_INPUT_PULLUP,
                   D_INPUT_PULLDOWN,
                   A_INPUT,
                   D_OUTPUT,
                   A_OUTPUT,
                   D_OSC,
                   A_OSC,
                   PINMODE_MAX };

DeclareIntParam(pin, -1);
DeclareIntParam(mode, IOComponent::D_OUTPUT);
DeclareBoolParam(inverted, false);

bool ledCAttached;
int curPin;

DeclareFloatParam(value, 0);
float prevValue;

const String modeOptions[PINMODE_MAX]{"Digital Input", "Digital Input Pullup", "Digital Input Pulldown", "Analog Input", "Digital Output", "Analog Output", "Digital Oscillator", "Analog Oscillator"};

virtual void setupInternal(JsonObject o) override;
virtual bool initInternal() override;
virtual void updateInternal() override;
virtual void clearInternal() override;

virtual void setupPin();
void updatePin();

void paramValueChangedInternal(void *param) override;

HandleSetParamInternalStart
    CheckAndSetParam(pin);
CheckAndSetEnumParam(mode, modeOptions, PINMODE_MAX);
CheckAndSetParam(inverted);
CheckAndSetParam(value);
HandleSetParamInternalEnd;

CheckFeedbackParamInternalStart
    CheckAndSendParamFeedback(value);
CheckFeedbackParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(pin);
FillSettingsParam(mode);
FillSettingsParam(inverted);

if(mode == D_OUTPUT || mode == A_OUTPUT || mode == A_OSC || mode == D_OSC)
{
    FillSettingsParam(value);
}
{
    FillSettingsParam(value);
}
FillSettingsInternalEnd

    FillOSCQueryInternalStart
        FillOSCQueryIntParam(pin);
FillOSCQueryEnumParam(mode, modeOptions, PINMODE_MAX);
FillOSCQueryBoolParam(inverted);
FillOSCQueryRangeParam(value, 0, 1);
FillOSCQueryInternalEnd

    EndDeclareComponent;

// Manager

DeclareComponentManager(IO, IO, gpio, gpio, IO_MAX_COUNT)
EndDeclareComponent