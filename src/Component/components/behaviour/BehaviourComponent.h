#pragma once

#ifndef BEHAVIOUR_MAX_COUNT
#define BEHAVIOUR_MAX_COUNT 4
#endif

DeclareComponent(Behaviour, "behaviour", )

    enum Comparator {
        EQUAL,
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL,
        OPERATOR_MAX
    };

const String operatorOptions[OPERATOR_MAX] = {
    "Equal",
    "Greater",
    "Greater or Equal",
    "Less",
    "Less or Equal"};

DeclareStringParam(paramName, "");
DeclareIntParam(comparator, EQUAL);
DeclareFloatParam(compareValue, 0.f);
DeclareFloatParam(validationTime, 0.f);
DeclareBoolParam(alwaysTrigger, false);
DeclareBoolParam(valid, false);

enum Action
{
    None,
    Shutdown,
    LaunchSeq,
    LaunchScript,
    LaunchCommand,
    ActionMax
};

const String triggerActionOptions[ActionMax] = {
    "None",
    "Shutdown",
    "Launch Sequence",
    "Launch Script",
    "Launch Command"};

DeclareEnumParam(triggerAction, None);
DeclareStringParam(triggerValue, "");

void *targetParam;
int listenerIndex = -1;
float timeAtValidation = 0.f;
bool delayedValidation = false;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;

void updateTargetParameter();

// void onParameterEventInternal(const ParameterEvent &e) override;

// void onTargetParameterChanged(const ParameterEvent &e);

void trigger();

DeclareComponentEventTypes(CommandLaunched);
DeclareComponentEventNames("CommandLaunched");

;

DeclareComponentManager(Behaviour, BEHAVIOUR, behaviours, behaviour, BEHAVIOUR_MAX_COUNT)

    void onChildComponentEvent(const ComponentEvent &e) override;
DeclareComponentEventTypes(CommandLaunched);
DeclareComponentEventNames("CommandLaunched");
EndDeclareComponent