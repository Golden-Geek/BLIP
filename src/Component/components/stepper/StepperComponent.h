#pragma once

#ifndef STEPPER_DEFAULT_DIR_PIN
#define STEPPER_DEFAULT_DIR_PIN -1
#endif

#ifndef STEPPER_DEFAULT_STEP_PIN
#define STEPPER_DEFAULT_STEP_PIN -1
#endif

#ifndef STEPPER_DEFAULT_EN_PIN
#define STEPPER_DEFAULT_EN_PIN -1
#endif

DeclareComponent(Stepper, "stepper", )

    static FastAccelStepperEngine engine;
static bool engineIsInit;
FastAccelStepper *stepper = NULL;

DeclareIntParam(dirPin, STEPPER_DEFAULT_DIR_PIN);
DeclareIntParam(stepPin, STEPPER_DEFAULT_STEP_PIN);
DeclareIntParam(enPin, STEPPER_DEFAULT_EN_PIN);

DeclareFloatParam(accel, 0);
DeclareFloatParam(speed, 0);
DeclareFloatParam(position, 0);

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void onEnabledChanged() override;
void paramValueChangedInternal(void *param) override;


    EndDeclareComponent