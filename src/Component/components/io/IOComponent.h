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

#ifndef IO_PULL_DEBOUNCE
#define IO_PULL_DEBOUNCE 5 // denoising, needs five reads to validate a change
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
                   TOUCH_INPUT,
                   PINMODE_MAX };

DeclareIntParam(pin, -1);
DeclareEnumParam(mode, IOComponent::D_OUTPUT);
DeclareBoolParam(inverted, false);

bool ledCAttached;
int curPin;
long lastUpdateTime;

DeclareFloatParam(value, 0);
float prevValue;

int debounceCount = 0;

const std::string modeOptions[PINMODE_MAX]{"Digital Input", "Digital Input Pullup", "Digital Input Pulldown", "Analog Input", "Digital Output", "Analog Output", "Digital Oscillator", "Analog Oscillator", "Touch"};

virtual void setupInternal(JsonObject o) override;
virtual bool initInternal() override;
virtual void updateInternal() override;
virtual void clearInternal() override;

virtual void setupPin();
void updatePin();

void paramValueChangedInternal(void *param) override;

    EndDeclareComponent;

// Manager

DeclareComponentManager(IO, IO, gpio, gpio, IO_MAX_COUNT)
    EndDeclareComponent