#include "UnityIncludes.h"

ImplementManagerSingleton(IO);

void IOComponent::setupInternal(JsonObject o)
{
    curPin = -1;
    pwmChannel = -1;
    ledCAttached = false;

    if (index == 0 && pin == -1)
    {
        pin = IO_DEFAULT_PIN;
        mode = IO_DEFAULT_MODE;
    }

    AddIntParamConfig(pin);
    AddIntParamConfig(mode);

    AddBoolParamConfig(inverted);

    AddFloatParam(value);

    prevValue = value;
}

bool IOComponent::initInternal()
{
    setupPin();
    updatePin();

    return true;
}

void IOComponent::updateInternal()
{
    updatePin();
}

void IOComponent::clearInternal()
{
}

void IOComponent::paramValueChangedInternal(void *param)
{
    if (param == &pin || param == &mode)
        setupPin();
    if (param == &value)
    {
        if (mode == D_OUTPUT || mode == A_OUTPUT)
        {
            updatePin();
        }
    }
}

void IOComponent::setupPin()
{
    if (curPin != -1) // prevPin was a PWM pin
    {
        // NDBG("Detach Pin " + String(curPin));
        if (ledCAttached)
            ledcDetach(curPin);
    }

    curPin = pin;

    if (curPin > 0)
    {
        int m = mode;

        int pinm = -1;
        switch (m)
        {
        case D_INPUT:
        case A_INPUT:

            pinm = INPUT;
            break;

        case D_INPUT_PULLUP:
            // DBG("INPUT_PULLUP");
            pinm = INPUT_PULLUP;
            break;

        case D_INPUT_PULLDOWN:
            // DBG("INPUT_PULLDOWN");
            pinm = INPUT_PULLDOWN;
            break;

        case D_OUTPUT:
        case D_OSC:
            pinm = OUTPUT;
            break;

        default:
            break;
        }

        if (pinm != -1)
        {

            pinMode(curPin, pinm);
        }
        else
        {
            if (m == A_OUTPUT || m == A_OSC)
            {
                // NDBG("Attach pin " + String(curPin) + " to PWM");
                ledcAttach(curPin, 5000, 10);
                ledCAttached = true;
            }
        }
    }
}

void IOComponent::updatePin()
{
    if (pin == -1)
        return;

    int m = mode;
    switch (m)
    {
    case D_INPUT:
    case D_INPUT_PULLUP:
    case D_INPUT_PULLDOWN:
    {
        bool val = digitalRead(pin);

        if (inverted)
            val = !val;

        if (value != val)
        {
            SetParam(value, val);
        }
        // SetParam(value, val);
    }
    break;

    case D_OUTPUT:
    case A_OUTPUT:
    {
        if (prevValue != value)
        {
            if (m == D_OUTPUT)
            {
                digitalWrite(pin, inverted ? !value : value);
            }
            else
            {
                if (pin != -1)
                {
                    uint32_t v = value * 1024;
                    // NDBG("Set PWM with value " + String(v));
                    ledcWrite(pin, v);
                }
            }

            prevValue = value;
        }
    }
    break;

    case A_INPUT:
    {
        float v = analogRead(pin) / 4095.0f;
        if (inverted)
            v = 1 - v;
        SetParam(value, v);
    }
    break;

    case D_OSC:
    {
        bool v = (millis() % (int)(value * 1000)) > (value * 300);
        if (value == 0)
            v = 0;
        else if (value == 1)
            v = 1;

        if (inverted)
            v = !v;
        digitalWrite(pin, v);
    }
    break;

    case A_OSC:
    {
        if (pin != -1)
        {
            float sv = sin(millis() * value) * 0.5f + 0.5f;
            uint32_t v = sv * 1024;
// NDBG("Set PWM with value " + String(v));
            ledcWrite(pin, v);
        }
    }
    break;
    }
}