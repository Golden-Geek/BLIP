#pragma once

#define BATTERY_CHECK_INTERVAL 100
#define BATTERY_AVERAGE_WINDOW 10

#ifndef BATTERY_DEFAULT_PIN
#define BATTERY_DEFAULT_PIN -1
#endif

#ifndef BATTERY_DEFAULT_CHARGE_PIN
#define BATTERY_DEFAULT_CHARGE_PIN -1
#endif

#ifndef BATTERY_DEFAUT_RAW_MIN
#define BATTERY_DEFAUT_RAW_MIN 0
#endif

#ifndef BATTERY_DEFAULT_RAW_MAX
#define BATTERY_DEFAULT_RAW_MAX 4095
#endif

#ifndef BATTERY_DEFAULT_LOW_VOLTAGE
#define BATTERY_DEFAULT_LOW_VOLTAGE 3.5f
#endif

#ifndef BATTERY_READ_MILLIVOLTS_MULTIPLIER
#define BATTERY_READ_MILLIVOLTS_MULTIPLIER 1
#endif


DeclareComponentSingleton(Battery, "battery", )

    DeclareIntParam(batteryPin, BATTERY_DEFAULT_PIN);
DeclareIntParam(chargePin, BATTERY_DEFAULT_CHARGE_PIN);
DeclareIntParam(rawMin, BATTERY_DEFAUT_RAW_MIN);
DeclareIntParam(rawMax, BATTERY_DEFAULT_RAW_MAX);
DeclareFloatParam(lowBatteryThreshold, BATTERY_DEFAULT_LOW_VOLTAGE);

DeclareBoolParam(sendFeedback, true);

DeclareFloatParam(batteryLevel, 4.2f);
DeclareBoolParam(charging, false);

long lastBatteryCheck = 0;
long lastBatterySet = 0;
long timeAtLowBattery = 0;
float values[BATTERY_AVERAGE_WINDOW];
int valuesIndex = 0;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

Color getBatteryColor();

float getVoltage_c6(float val);

DeclareComponentEventTypes(CriticalBattery);
DeclareComponentEventNames("CriticalBattery");

CheckFeedbackParamInternalStart
    CheckAndSendParamFeedback(batteryLevel);
CheckFeedbackParamInternalEnd;

HandleSetParamInternalStart
    CheckAndSetParam(batteryPin);
CheckAndSetParam(chargePin);
CheckAndSetParam(rawMin);
CheckAndSetParam(rawMax);
CheckAndSetParam(lowBatteryThreshold);
CheckAndSetParam(sendFeedback);
CheckAndSetParam(batteryLevel);
CheckAndSetParam(charging);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(batteryPin);
FillSettingsParam(chargePin);
FillSettingsParam(rawMin);
FillSettingsParam(rawMax);
FillSettingsParam(lowBatteryThreshold);
FillSettingsParam(sendFeedback);
FillSettingsParam(batteryLevel);
FillSettingsParam(charging);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryIntParam(batteryPin);
FillOSCQueryIntParam(chargePin);
FillOSCQueryIntParam(rawMin);
FillOSCQueryIntParam(rawMax);
FillOSCQueryRangeParam(lowBatteryThreshold, 3.3, 4.2)
    FillOSCQueryBoolParam(sendFeedback);
FillOSCQueryRangeParamReadOnly(batteryLevel, lowBatteryThreshold, 4.2f);
FillOSCQueryBoolParamReadOnly(charging);
FillOSCQueryInternalEnd

    EndDeclareComponent