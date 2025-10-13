#pragma once

#define BATTERY_CHECK_INTERVAL 100
#define BATTERY_AVERAGE_WINDOW 30

#ifndef BATTERY_DEFAULT_PIN
#define BATTERY_DEFAULT_PIN -1
#endif

#ifndef BATTERY_DEFAULT_CHARGE_PIN
#define BATTERY_DEFAULT_CHARGE_PIN -1
#endif

#ifndef BATTERY_CHARGE_PIN_MODE
#define BATTERY_CHARGE_PIN_MODE INPUT_PULLUP
#endif

#ifndef BATTERY_CHARGE_PIN_INVERTED
#define BATTERY_CHARGE_PIN_INVERTED false
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

#ifndef BATTERY_MIN_VOLTAGE
#define BATTERY_MIN_VOLTAGE 3.3f
#endif

#ifndef BATTERY_MAX_VOLTAGE
#define BATTERY_MAX_VOLTAGE 4.2f
#endif

#ifndef BATTERY_READ_MILLIVOLTS_MULTIPLIER
#define BATTERY_READ_MILLIVOLTS_MULTIPLIER 1
#endif

DeclareComponentSingleton(Battery, "battery", )

    DeclareIntParam(batteryPin, BATTERY_DEFAULT_PIN);
DeclareIntParam(chargePin, BATTERY_DEFAULT_CHARGE_PIN);
#ifndef BATTERY_READ_MILLIVOLTS
DeclareIntParam(rawMin, BATTERY_DEFAUT_RAW_MIN);
DeclareIntParam(rawMax, BATTERY_DEFAULT_RAW_MAX);
#endif
DeclareFloatParam(lowBatteryThreshold, BATTERY_DEFAULT_LOW_VOLTAGE);

DeclareIntParam(feedbackInterval, 3); // seconds

DeclareFloatParam(batteryLevel, 1.0f);
DeclareFloatParam(voltage, BATTERY_MAX_VOLTAGE);
DeclareBoolParam(charging, false);
DeclareIntParam(shutdownChargeNoSignal, 0);      // seconds, 0 = disabled
DeclareIntParam(shutdownChargeSignalTimeout, 0); // seconds, 0 = disabled

long lastBatteryCheck = 0;
long lastBatterySet = 0;
long timeAtLowBattery = 0;
float values[BATTERY_AVERAGE_WINDOW];
int valuesIndex = 0;

bool readChargePinOnNextCheck = false;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void readChargePin();
void readBatteryLevel();

void checkShouldAutoShutdown();

bool isBatteryLow() const { return voltage < lowBatteryThreshold; }

Color getBatteryColor();

DeclareComponentEventTypes(CriticalBattery);
DeclareComponentEventNames("CriticalBattery");

CheckFeedbackParamInternalStart if (feedbackInterval <= 0) return false;
CheckAndSendParamFeedback(batteryLevel);
CheckAndSendParamFeedback(voltage);
CheckAndSendParamFeedback(charging);
CheckFeedbackParamInternalEnd;

HandleSetParamInternalStart
    CheckAndSetParam(batteryPin);
CheckAndSetParam(chargePin);
#ifndef BATTERY_READ_MILLIVOLTS
CheckAndSetParam(rawMin);
CheckAndSetParam(rawMax);
#endif
CheckAndSetParam(lowBatteryThreshold);
CheckAndSetParam(feedbackInterval);
CheckAndSetParam(charging);
CheckAndSetParam(shutdownChargeNoSignal);
CheckAndSetParam(shutdownChargeSignalTimeout);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(batteryPin);
FillSettingsParam(chargePin);
#ifndef BATTERY_READ_MILLIVOLTS
FillSettingsParam(rawMin);
FillSettingsParam(rawMax);
#endif
FillSettingsParam(lowBatteryThreshold);
FillSettingsParam(feedbackInterval);
FillSettingsParam(charging);
FillSettingsParam(shutdownChargeNoSignal);
FillSettingsParam(shutdownChargeSignalTimeout);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryIntParam(batteryPin);
FillOSCQueryIntParam(chargePin);
#ifndef BATTERY_READ_MILLIVOLTS
FillOSCQueryIntParam(rawMin);
FillOSCQueryIntParam(rawMax);
#endif
FillOSCQueryRangeParam(lowBatteryThreshold, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE)
    FillOSCQueryIntParam(feedbackInterval);
FillOSCQueryRangeParamReadOnly(batteryLevel, 0, 1);
FillOSCQueryRangeParamReadOnly(voltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
FillOSCQueryBoolParamReadOnly(charging);
FillOSCQueryIntParam(shutdownChargeNoSignal);
FillOSCQueryIntParam(shutdownChargeSignalTimeout);
FillOSCQueryInternalEnd

    EndDeclareComponent