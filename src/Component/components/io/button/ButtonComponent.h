#pragma once

#define LONGPRESS_TIME 500      // more than 500ms is long press
#define VERYLONGPRESS_TIME 1500 // more than 1500ms is very long press
#define SHORTPRESS_TIME 500     // less than 500ms is short press
#define MULTIPRESS_TIME 300     // each new press shorter than 500ms after the previous one will increase the multiclick

#ifndef BUTTON_DEFAULT_SHUTDOWN
#ifdef USE_BATTERY
#define BUTTON_DEFAULT_SHUTDOWN true
#else
#define BUTTON_DEFAULT_SHUTDOWN false
#endif
#endif

#ifndef BUTTON_MAX_COUNT
#define BUTTON_MAX_COUNT 4
#endif

#ifndef BUTTON_DEFAULT_PIN
#define BUTTON_DEFAULT_PIN -1
#endif

#ifndef BUTTON_DEFAULT_MODE
#define BUTTON_DEFAULT_MODE D_INPUT
#endif

#ifndef BUTTON_DEFAULT_INVERTED
#define BUTTON_DEFAULT_INVERTED false
#endif

class ButtonComponent : public IOComponent
{
public:
    ButtonComponent(const std::string &name = "button", bool _enabled = false, int index = 0) : IOComponent(name, _enabled, index) {}

    long timeAtPress = 0;
    bool wasPressedAtBoot = false;
    bool releasedAfterBootPress = false;

    DeclareBoolParam(canShutDown, BUTTON_DEFAULT_SHUTDOWN);
    DeclareIntParam(multiPressCount, 0);
    DeclareBoolParam(longPress, false);
    DeclareBoolParam(veryLongPress, false);

    void setupInternal(JsonObject o) override;
    void updateInternal() override;
    void paramValueChangedInternal(ParamInfo *param) override;

#if USE_SCRIPT
    // LinkScriptFunctionsStart
    //     LinkScriptFunction(ButtonComponent, getState, i, );
    // LinkScriptFunction(ButtonComponent, getMultipress, i, );
    // LinkScriptFunctionsEnd

    // DeclareScriptFunctionReturn0(ButtonComponent, getState, uint32_t)
    // {
    //     return veryLongPress ? 3 : longPress ? 2
    //                                          : value;
    // }

    // DeclareScriptFunctionReturn0(ButtonComponent, getMultipress, uint32_t) { return multiPressCount; }
#endif
};

DeclareComponentManager(Button, BUTTON, buttons, button, BUTTON_MAX_COUNT)
    EndDeclareComponent
