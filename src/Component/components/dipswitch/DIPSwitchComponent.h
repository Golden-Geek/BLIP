#pragma once
#include <TCA6408.h>

DeclareComponent(DIPSwitch, "dipswitch", )
DeclareIntParam(value, 0);

TCA6408 tca6408;

long lastReadTime = 0;
const long readInterval = 500; // in ms

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void updateValue();

// HandleSetParamInternalStart
//     HandleSetParamInternalEnd;

// CheckFeedbackParamInternalStart;
// CheckAndSendParamFeedback(value);
// CheckFeedbackParamInternalEnd;

// FillSettingsInternalStart;
// FillSettingsInternalEnd;

// FillOSCQueryInternalStart;
// FillOSCQueryIntParamReadOnly(value);
// FillOSCQueryInternalEnd;

EndDeclareComponent