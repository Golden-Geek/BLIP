#include "UnityIncludes.h"

void DIPSwitchComponent::setupInternal(JsonObject o)
{
    AddIntParamFeedback(value);
}

bool DIPSwitchComponent::initInternal()
{
    pinMode(DIPSWITCH_EXTRA_PIN, INPUT_PULLUP);

    Wire.setPins(DIPSWITCH_I2C_PIN_SDA, DIPSWITCH_I2C_PIN_SCL);
    tca6408.setup(Wire, TCA6408::DEVICE_ADDRESS_0);

    return true;
}


void DIPSwitchComponent::updateInternal()
{
    if (millis() - lastReadTime >= readInterval)
    {
        lastReadTime = millis();
        updateValue();
    }
}

void DIPSwitchComponent::updateValue()
{
    // dip switch is 8 bit + 1 extra pin for bit 9
    int newValue = tca6408.readInputRegister();
    newValue = ~newValue & 0xFF;

    // low values are ON for the dip switch
    bool extraPinState = digitalRead(DIPSWITCH_EXTRA_PIN);
    if (extraPinState == LOW) newValue |= 0x100; // set bit 9

    if(newValue == value)  return;
    SetParam(value, newValue);
}
