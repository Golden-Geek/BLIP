#pragma once

#define LEDSTRIP_NUM_USER_LAYERS 3
#define USE_BAKELAYER 1

#ifdef USE_STREAMING
#define USE_STREAMINGLAYER 1
#else
#define USE_STREAMINGLAYER 0
#endif

#ifdef USE_SCRIPT
#define USE_SCRIPTLAYER 1
#else
#define USE_SCRIPTLAYER 0
#endif

#define USE_SYSTEMLAYER 1

class LedStripComponent : public Component
{
public:
    LedStripComponent(const String &name = "strip", bool enabled = true, int index = 0) : Component(name, enabled, index)
#if USE_BAKELAYER
                                                                                          ,
                                                                                          bakeLayer(this)
#endif
#if USE_STREAMINGLAYER
                                                                                          ,
                                                                                          streamLayer(this)
#endif
#if USE_SCRIPTLAYER
                                                                                          ,
                                                                                          scriptLayer("scriptLayer", LedStripLayer::Type::ScriptType, this)
#endif
#if USE_SYSTEMLAYER
                                                                                          ,
                                                                                          systemLayer(this)
#endif
#ifdef USE_FX
                                                                                          ,
                                                                                          fx(this)
#endif
                                                                                          
    {
    }

    ~LedStripComponent() {}
    virtual String getTypeString() const override { return "ledstrip"; }

    DeclareIntParam(count, LED_DEFAULT_COUNT);
    DeclareIntParam(dataPin, LED_DEFAULT_DATA_PIN);
    DeclareIntParam(clkPin, LED_DEFAULT_CLK_PIN);
    DeclareIntParam(enPin, LED_DEFAULT_EN_PIN);

    DeclareFloatParam(brightness, LED_DEFAULT_BRIGHTNESS);
    DeclareIntParam(maxPower, LED_DEFAULT_MAX_POWER);
    DeclareBoolParam(colorCorrection, LED_DEFAULT_CORRECTION);

    // mapping
    DeclareBoolParam(invertStrip, LED_DEFAULT_INVERT_DIRECTION);

    enum MultiLedMode
    {
        FullColor,
        SingleColor,
        TwoColors,
        MultiLedModeMax
    };

    const String multiLedModeOptions[MultiLedModeMax] = {
        "Full Color",
        "Single Color",
        "Two Colors"};

    DeclareIntParam(multiLedMode, LED_DEFAULT_MULTILED_MODE);

    int numColors = 0;

    Color colors[LED_MAX_COUNT];
    uint8_t ditherFrameCounter = 0;

    // user layers, may be more than one later
#if USE_BAKELAYER
    LedStripPlaybackLayer bakeLayer;
#endif
#if USE_STREAMINGLAYER
    LedStripStreamLayer streamLayer;
#endif
#if USE_SCRIPTLAYER
    LedStripLayer scriptLayer;
#endif
#if USE_SYSTEMLAYER
    LedStripSystemLayer systemLayer;
#endif

#if USE_FX
    FXComponent fx;
#endif

    LedStripLayer *userLayers[LEDSTRIP_NUM_USER_LAYERS];

#ifdef LED_USE_FASTLED
    CRGB leds[LED_MAX_COUNT];
#elif defined LED_USE_CUSTOM_FUNC
    ws2812 wleds;
#else
    Adafruit_NeoPixel *neoPixelStrip;
    Adafruit_DotStar *dotStarStrip;
#endif

    bool updatingLeds = false;
    bool showingLeds = false;
    bool shouldStopTask = false;

    void setupInternal(JsonObject o) override;
    bool initInternal() override;
    void updateInternal() override;
    
    void clearInternal() override;
    void shutdown();

    void setupLeds();

    void setBrightness(float val);
    void updateCorrection();

    int getNumColors() const;
    int getColorIndex(int i) const;

    void paramValueChangedInternal(void *param) override;
    void onEnabledChanged() override;

    void setStripPower(bool value);

    // Layer functions
    void processLayer(LedStripLayer *layer);

    // Color functions
    void clearColors();
    void showLeds();
    static void showLedsAsync(void *);

    uint8_t getDitheredBrightness(uint8_t brightness, uint8_t frame);

    int ledMap(int index) const;

    HandleSetParamInternalStart
        CheckAndSetParam(count);
    CheckAndSetParam(dataPin);
    CheckAndSetParam(clkPin);
    CheckAndSetParam(enPin);
    CheckAndSetParam(brightness);
    CheckAndSetParam(invertStrip);
    CheckAndSetEnumParam(multiLedMode, multiLedModeOptions, MultiLedModeMax);
    CheckAndSetParam(maxPower);
    CheckAndSetParam(colorCorrection);
    HandleSetParamInternalEnd;

    FillSettingsInternalStart
        FillSettingsParam(count);
    FillSettingsParam(dataPin);
    FillSettingsParam(clkPin);
    FillSettingsParam(enPin);
    FillSettingsParam(brightness);
    FillSettingsParam(invertStrip);
    FillSettingsParam(maxPower);
    FillSettingsParam(multiLedMode);
    FillSettingsParam(colorCorrection);
    FillSettingsInternalEnd

        FillOSCQueryInternalStart
            FillOSCQueryIntParam(count);
    FillOSCQueryIntParam(dataPin);
    FillOSCQueryIntParam(clkPin);
    FillOSCQueryIntParam(enPin);
    FillOSCQueryRangeParam(brightness, 0, 2);
    FillOSCQueryBoolParam(invertStrip);
    FillOSCQueryEnumParam(multiLedMode, multiLedModeOptions, MultiLedModeMax);
    FillOSCQueryIntParam(maxPower);
    FillOSCQueryBoolParam(colorCorrection);
    FillOSCQueryInternalEnd
};

DeclareComponentManager(LedStrip, LEDSTRIP, leds, strip, LEDSTRIP_MAX_COUNT)

#ifdef USE_SCRIPT

    LinkScriptFunctionsStart
    LinkScriptFunction(LedStripManagerComponent, clear, v, );
LinkScriptFunction(LedStripManagerComponent, fillAll, v, i);
LinkScriptFunction(LedStripManagerComponent, fillRange, v, iff);
LinkScriptFunction(LedStripManagerComponent, fillRGB, v, iii);
LinkScriptFunction(LedStripManagerComponent, fillHSV, v, fff);

LinkScriptFunction(LedStripManagerComponent, point, v, iff);
LinkScriptFunction(LedStripManagerComponent, pointRGB, v, iiiff);
LinkScriptFunction(LedStripManagerComponent, pointHSV, v, fffff);

LinkScriptFunction(LedStripManagerComponent, set, v, ii);
LinkScriptFunction(LedStripManagerComponent, setRGB, v, iiii);
LinkScriptFunction(LedStripManagerComponent, setHSV, v, ifff);

LinkScriptFunction(LedStripManagerComponent, get, i, i);

LinkScriptFunction(LedStripManagerComponent, setBlendMode, v, iii);

LinkScriptFunctionsEnd

DeclareScriptFunctionVoid0(LedStripManagerComponent, clear)
{
    items[0]->scriptLayer.clearColors();
}
DeclareScriptFunctionVoid1(LedStripManagerComponent, fillAll, uint32_t) { items[0]->scriptLayer.fillAll(arg1); }
DeclareScriptFunctionVoid3(LedStripManagerComponent, fillRange, uint32_t, float, float) { items[0]->scriptLayer.fillRange(arg1, arg2, arg3); }

DeclareScriptFunctionVoid3(LedStripManagerComponent, fillRGB, uint32_t, uint32_t, uint32_t) { items[0]->scriptLayer.fillAll(Color(arg1, arg2, arg3)); }
DeclareScriptFunctionVoid3(LedStripManagerComponent, fillHSV, float, float, float) { items[0]->scriptLayer.fillAll(Color::HSV(arg1, arg2, arg3)); }

DeclareScriptFunctionVoid3(LedStripManagerComponent, point, uint32_t, float, float) { items[0]->scriptLayer.point(arg1, arg2, arg3, false); }
DeclareScriptFunctionVoid5(LedStripManagerComponent, pointRGB, uint32_t, uint32_t, uint32_t, float, float) { items[0]->scriptLayer.point(Color(arg1, arg2, arg3), arg4, arg5, false); }
DeclareScriptFunctionVoid5(LedStripManagerComponent, pointHSV, float, float, float, float, float) { items[0]->scriptLayer.point(Color::HSV(arg1, arg2, arg3), arg4, arg5, false); }

DeclareScriptFunctionVoid2(LedStripManagerComponent, set, uint32_t, uint32_t) { items[0]->scriptLayer.setLed(arg1, arg2); }
DeclareScriptFunctionVoid4(LedStripManagerComponent, setRGB, uint32_t, uint32_t, uint32_t, uint32_t) { items[0]->scriptLayer.setLed(arg1, Color(arg2, arg3, arg4)); }
DeclareScriptFunctionVoid4(LedStripManagerComponent, setHSV, uint32_t, float, float, float) { items[0]->scriptLayer.setLed(arg1, Color::HSV(arg2, arg3, arg4)); }
DeclareScriptFunctionReturn1(LedStripManagerComponent, get, uint32_t, uint32_t) { return items[0]->scriptLayer.getLed(arg1).value; }

DeclareScriptFunctionVoid3(LedStripManagerComponent, setBlendMode, uint32_t, uint32_t, uint32_t) { return items[0]->userLayers[(int)arg2]->setBlendMode((LedStripLayer::BlendMode)arg3); }
#endif

void shutdown()
{
    for (int i = 0; i < count; i++)
    {
        items[i]->shutdown();
    }
}

EndDeclareComponent