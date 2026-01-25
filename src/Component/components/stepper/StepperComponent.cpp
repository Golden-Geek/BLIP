#include "UnityIncludes.h"

FastAccelStepperEngine StepperComponent::engine = FastAccelStepperEngine();
bool StepperComponent::engineIsInit = false;

void StepperComponent::setupInternal(JsonObject o)
{

    AddIntParamConfig(stepPin);
    AddIntParamConfig(dirPin);
    AddIntParamConfig(enPin);

    AddFloatParamConfig(accel);
    AddFloatParamConfig(speed);
    AddFloatParamFeedback(position);
}

bool StepperComponent::initInternal()
{

    if (!engineIsInit)
    {
        NDBG("Init Stepper Engine");
        engine.init(1);
    }

    if (stepPin > -1)
    {
        stepper = engine.stepperConnectToPin(stepPin);
        if (stepper)
        {
            stepper->setDirectionPin(dirPin);
            if (enPin > -1)
            {
                NDBG("Set enable pin " + std::to_string(enPin));
                delay(200);
                stepper->setEnablePin(enPin);
                stepper->enableOutputs();
                // stepper->setAutoEnable(true);
            }

            stepper->setSpeedInHz(speed);    // 500 steps/s
            stepper->setAcceleration(accel); // 100 steps/s

            NDBG("Connected stepper on stepPin " + std::to_string(stepPin) + ", dirPin " + std::to_string(dirPin) + ", enPin " + std::to_string(enPin));
        }
        else
        {
            NDBG("Could not connect stepper on stepPin " + std::to_string(stepPin));
            return false;
        }
    }
    else
    {
        NDBG("stepPin is not defined, not connecting stepper.");
        return false;
    }

    return true;
}

void StepperComponent::updateInternal()
{
}

void StepperComponent::clearInternal()
{
    stepper->disableOutputs();
}

void StepperComponent::paramValueChangedInternal(ParamInfo *paramInfo)
{
    if (!isInit)
        return;

    void *param = paramInfo->ptr;

    if (param == &position)
    {
        NDBG("Move to position " + std::to_string(position));
        stepper->moveTo(position);
    }
    else if (param == &speed)
    {
        NDBG("Set speed to " + std::to_string(speed));
        stepper->setSpeedInHz(speed);
    }
    else if (param == &accel)
    {
        NDBG("Set acceleration to " + std::to_string(accel));
        stepper->setAcceleration(accel);
    }
}

void StepperComponent::onEnabledChanged()
{
    if (!isInit)
        return;
    if (enabled)
        stepper->enableOutputs();
    else
        stepper->disableOutputs();
}