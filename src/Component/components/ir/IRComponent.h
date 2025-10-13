#pragma once

#ifndef IR_DEFAULT_PIN
#define IR_DEFAULT_PIN -1
#endif

#ifndef IR_DEFAULT_PIN2
#define IR_DEFAULT_PIN2 -1
#endif

DeclareComponent(IR, "ir", )

    bool ledCAttached1;
int curPin1;
bool ledCAttached2;
int curPin2;

float prevValue;

DeclareIntParam(pin1, IR_DEFAULT_PIN);
DeclareIntParam(pin2, IR_DEFAULT_PIN2);
DeclareFloatParam(value, 0);
DeclareBoolParam(keepValueOnReboot, false);

virtual void setupInternal(JsonObject o) override;
virtual bool initInternal() override;
virtual void updateInternal() override;
virtual void clearInternal() override;

virtual void setupPins();
void updatePins();

void paramValueChangedInternal(void *param) override;

HandleSetParamInternalStart
    CheckAndSetParam(pin1);
CheckAndSetParam(pin2);
CheckAndSetParam(value);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(pin1);
FillSettingsParam(pin2);
if (keepValueOnReboot)
    FillSettingsParam(value);
FillSettingsParam(keepValueOnReboot);
FillSettingsInternalEnd

    FillOSCQueryInternalStart
        FillOSCQueryIntParam(pin1);
FillOSCQueryIntParam(pin2);
FillOSCQueryRangeParam(value, 0, 1);
FillOSCQueryBoolParam(keepValueOnReboot);
FillOSCQueryInternalEnd

    EndDeclareComponent;

// Manager