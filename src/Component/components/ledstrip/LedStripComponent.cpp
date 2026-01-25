#include "UnityIncludes.h"

#define SetupFastLED(Type, DataPin, count) FastLED.addLeds<Type, DataPin>(leds, count)

ImplementManagerSingleton(LedStrip);

void LedStripComponent::setupInternal(JsonObject o)
{
    setCustomUpdateRate(60, o); // 60 fps customizable
// init
#ifdef LED_USE_FASTLED
#else
    neoPixelStrip = NULL;
#endif

    for (int i = 0; i < LEDSTRIP_NUM_USER_LAYERS; i++)
        userLayers[i] = NULL;

    if (index == 1)
    {
        dataPin = LED2_DEFAULT_DATA_PIN;
        clkPin = LED2_DEFAULT_CLK_PIN;
        enPin = LED2_DEFAULT_EN_PIN;
    }

    AddIntParamConfig(count);
#ifdef LED_FIXED_COUNT
    setParamConfig(&count, false);
    setParamFeedback(&count, true);
#endif
    AddIntParamConfig(dataPin);
    AddIntParamConfig(enPin);
    AddIntParamConfig(clkPin);
    AddFloatParam(brightness);
    AddBoolParamConfig(invertStrip);
    AddEnumParamConfig(multiLedMode, multiLedModeOptions, MultiLedModeMax);
    AddIntParamConfig(maxPower);
    AddBoolParam(colorCorrection);

    numColors = count;

#if USE_BAKELAYER
    AddOwnedComponent(&bakeLayer);
    userLayers[0] = &bakeLayer;
#endif
#if USE_STREAMINGLAYER
    AddOwnedComponent(&streamLayer);
    userLayers[1] = &streamLayer;
#endif
#if USE_SCRIPTLAYER
    AddOwnedComponent(&scriptLayer);
    userLayers[2] = &scriptLayer;
#endif
#if USE_SYSTEMLAYER
    AddOwnedComponent(&systemLayer);
#endif

#if USE_FX
    AddOwnedComponent(&fx);
#endif
}

bool LedStripComponent::initInternal()
{
    setupLeds();
    return true;
}

void LedStripComponent::setupLeds()
{
    if (!enabled)
        return;

    if (enPin > 0)
    {
        // NDBG("Setting Led Enable pin : " + std::to_string(enPin));
        pinMode(enPin, OUTPUT);
        gpio_set_level(gpio_num_t(enPin), HIGH); // enable LEDs
    }

    if (count == 0 || dataPin == 0)
    {
        NDBG("Ledstrip pin and count have not been set");
        return;
    }

    // colors = (Color *)malloc(count * sizeof(Color));
    memset(colors, 0, LED_MAX_COUNT * sizeof(Color));

#ifdef LED_USE_FASTLED
    if (index == 0)
    {
#if LED_DEFAULT_CLK_PIN != -1
        NDBG("Using FastLED with DotStar strip on pin " + std::to_string(LED_DEFAULT_DATA_PIN) + " and clk pin " + std::to_string(LED_DEFAULT_CLK_PIN));
        FastLED.addLeds<LED_DEFAULT_TYPE, LED_DEFAULT_DATA_PIN, LED_DEFAULT_CLK_PIN, LED_DEFAULT_COLOR_ORDER>(leds, count);
#else
        NDBG("Using FastLED with NeoPixel strip");
        FastLED.addLeds<LED_DEFAULT_TYPE, LED_DEFAULT_DATA_PIN, LED_DEFAULT_COLOR_ORDER>(leds, count);
#endif
    }

#if LED2_DEFAULT_DATA_PIN != -1
    else if (index == 1)
    {
#if LED2_DEFAULT_CLK_PIN != -1
        NDBG("Using FastLED with DotStar strip for strip 2");
        FastLED.addLeds<LED2_DEFAULT_TYPE, LED_DEFAULT_DATA_PIN, LED_DEFAULT_CLK_PIN, LED2_DEFAULT_COLOR_ORDER>(leds, count);
#else
        NDBG("Using FastLED with NeoPixel strip for strip 2");
        FastLED.addLeds<LED2_DEFAULT_TYPE, LED2_DEFAULT_DATA_PIN, LED2_DEFAULT_COLOR_ORDER>(leds, count);
#endif
    }
#endif

    updateCorrection();
#else

    if (neoPixelStrip != NULL)
    {
        neoPixelStrip->ClearTo(RgbColor(0, 0, 0));
        neoPixelStrip->Show();
        delete neoPixelStrip;
        neoPixelStrip = NULL;
    }

    if (clkPin >= 0)
    {
        // neoPixelStrip = new NeoPixelBus<LED_DEFAULT_COLOR_ORDER, NeoPixelMethod>((uint16_t)count, (uint8_t)clkPin, (uint8_t)dataPin);
    }
    else
    {
        neoPixelStrip = new NeoPixelBus<LED_DEFAULT_COLOR_ORDER, LED_DEFAULT_TYPE>((uint16_t)count, (uint8_t)dataPin);
    }
    NDBG("Using NeoPixel strip on data pin " + std::to_string(dataPin));

    if (neoPixelStrip != NULL)
    {
        neoPixelStrip->Begin();
        neoPixelStrip->ClearTo(RgbColor(0, 0, 0));
        neoPixelStrip->Show();
    }
#endif

    setStripPower(true);
    showLeds(); // may fail when using files
}

void LedStripComponent::updateInternal()
{
#ifdef LED_USE_FASTLED
#else
    if (neoPixelStrip == NULL)
    {
        return; // not active
    }
#endif

    // all layer's internal colors are updated in Component's update() function

    if (doNotUpdate)
        return;

    clearColors();
    numColors = getNumColors();

    for (int i = 0; i < LEDSTRIP_NUM_USER_LAYERS; i++)
        processLayer(userLayers[i]);

#if USE_SYSTEMLAYER
    processLayer(&systemLayer);
#endif

#if USE_FX
    fx.process(colors);
#endif

    showLeds();
}

void LedStripComponent::clearInternal()
{
    NDBG("Clearing Led Strip Component");

    clearColors();
    showLeds();
    setStripPower(false);

#ifdef LED_USE_FASTLED
#else
    delete neoPixelStrip;
    neoPixelStrip = NULL;
#endif
}

void LedStripComponent::shutdown()
{
    setStripPower(true); // force turn on leds for shutdown section
}

void LedStripComponent::setBrightness(float val)
{
    SetParam(brightness, val);
}

void LedStripComponent::updateCorrection()
{
#if defined(LED_USE_FASTLED)
    if (colorCorrection)
        FastLED.setCorrection(TypicalLEDStrip);
    else
        FastLED.setCorrection(UncorrectedColor);
#endif
}

int LedStripComponent::getNumColors() const
{
    switch (multiLedMode)
    {
    case FullColor:
        return count;
    case SingleColor:
        return 1;
    case TwoColors:
        return 2;
    }

    return count;
}

int LedStripComponent::getColorIndex(int i) const
{
    switch (multiLedMode)
    {
    case FullColor:
        return i;
    case SingleColor:
        return 0;
    case TwoColors:
        return i < count / 2 ? 0 : 1;
    }

    return i;
}

void LedStripComponent::paramValueChangedInternal(void *param)
{
#ifdef LED_USE_FASTLED
    if (param == &colorCorrection)
        updateCorrection();
#else
    if (param == &count)
    {
        setupLeds();
    }
#endif
}

void LedStripComponent::onEnabledChanged()
{
    setStripPower(enabled);
}

void LedStripComponent::setStripPower(bool value)
{
    if (!isInit)
        return;

    if (value == currentStripPower)
        return;

    currentStripPower = value;

    if (enPin > 0)
    {
        NDBG("Set Strip Power " + std::to_string(value));
        gpio_set_level(gpio_num_t(enPin), value); // enable LEDs
    }
    else
    {
#if defined(LED_USE_FASTLED)
#else
#ifndef LED_EN_NO_OPEN_DRAIN
        pinMode(dataPin, value ? OUTPUT : OPEN_DRAIN);
#endif
#endif
    }
}

// Layer functions
void LedStripComponent::processLayer(LedStripLayer *layer)
{
    if (layer == NULL)
        return;

    if (!layer->enabled)
        return;

    // DBG("Processing Layer "+std::string(layer->name));

    for (int i = 0; i < count; i++)
    {
        int index = getColorIndex(i);
        Color c = layer->colors[index];
        // if(index == 0)
        // {
        //     DBG(" Layer color for led " + std::to_string(i) + ": " + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + ", " + std::to_string(c.a));
        // }

        LedStripLayer::BlendMode bm = (LedStripLayer::BlendMode)layer->blendMode;

        switch (bm)
        {
        case LedStripLayer::Add:
            colors[i] += c;
            break;

        case LedStripLayer::Multiply:
            colors[i] *= c;
            break;

        case LedStripLayer::Max:
            colors[i].r = max(colors[i].r, c.r);
            colors[i].g = max(colors[i].g, c.g);
            colors[i].b = max(colors[i].b, c.b);
            colors[i].a = max(colors[i].a, c.a);
            break;

        case LedStripLayer::Min:
            colors[i].r = min(colors[i].r, c.r);
            colors[i].g = min(colors[i].g, c.g);
            colors[i].b = min(colors[i].b, c.b);
            colors[i].a = min(colors[i].a, c.a);
            break;

        case LedStripLayer::Alpha:
        {

            float a = c.a / 255.0f;
            colors[i].r = colors[i].r + ((int)c.r - colors[i].r) * a;
            colors[i].g = colors[i].g + ((int)c.g - colors[i].g) * a;
            colors[i].b = colors[i].b + ((int)c.b - colors[i].b) * a;
            colors[i].a = max(colors[i].a, c.a);
        }
        break;

        default:
            break;
        }
    }
}

// Color functions

void LedStripComponent::clearColors()
{
    // for(int i=0;i<count;i++) colors[i] = Color(0,0,0,0);
    memset(colors, 0, sizeof(Color) * count);
}

void LedStripComponent::showLeds()
{
    // DBG("Showing Leds, led 0 color: " + std::to_string(colors[0].r) + ", " + std::to_string(colors[0].g) + ", " + std::to_string(colors[0].b) + ", " + std::to_string(colors[0].a));
    float targetBrightness = brightness * LED_BRIGHTNESS_FACTOR;
    if (maxPower > 0)
    {
        int totalAmpFullBrightness = 0;
        const float bMultiplier = LED_BRIGHTNESS_FACTOR * LED_LEDS_PER_PIXEL;

        for (int i = 0; i < count; i++)
        {
            Color c = colors[i];
            totalAmpFullBrightness += (((c.r * RED_MILLIAMP + c.g * GREEN_MILLIAMP + c.b * BLUE_MILLIAMP + DARK_MILLIAMP) / (c.a / 255.0f)) / 255.0f);
        }

        totalAmpFullBrightness *= bMultiplier;

        // NDBG("Total Amps at full brightness: " + std::to_string(totalAmpFullBrightness) + ", Max Power: " + std::to_string(maxPower));

        if (totalAmpFullBrightness > maxPower)
        {
            float maxOkBrightness = LED_BRIGHTNESS_FACTOR * maxPower / (float)totalAmpFullBrightness;
            targetBrightness = min(targetBrightness, maxOkBrightness);
            // NDBG("maxOkBrightness: " + std::to_string(maxOkBrightness) + ", Target brightness: " + std::to_string(targetBrightness));
        }
    }

    // DBG("Strip power is " + std::to_string(currentStripPower) + ", brightness : " + std::to_string(brightness) + ", target brightness: " + std::to_string(targetBrightness));

#ifdef LED_USE_FASTLED
    FastLED.setBrightness(min(int(targetBrightness * 255), 255));
    for (int i = 0; i < count; i++)
    {
        float a = colors[i].a / 255.0f;
        leds[ledMap(i)] = CRGB(colors[i].r * a, colors[i].g * a, colors[i].b * a);
    }
    // DBG("First Led " + std::to_string(ledMap(0)) + " color: " + std::to_string(colors[0].r) + ", " + std::to_string(colors[0].g) + ", " + std::to_string(colors[0].b) + ", " + std::to_string(colors[0].a));
    FastLED.show();
#else

    if (neoPixelStrip != NULL)
    {
        long startTime = millis();
        for (int i = 0; i < count; i++)
        {
            float bri = colors[i].a * targetBrightness;
            Rgb48Color linearColor = Rgb48Color(colors[i].r * bri, colors[i].g * bri, colors[i].b * bri);
            Rgb48Color correctedColor = colorCorrection ? colorGamma.Correct(linearColor) : linearColor;
            if (i == 0)
            {
                // DBG(" Led 0 color before correction: " + std::to_string((int)(colors[0].r * bri)) + ", " + std::to_string((int)(colors[0].g * bri)) + ", " + std::to_string((int)(colors[0].b * bri)) + ", after correction: " + std::to_string((int)correctedColor.R) + ", " + std::to_string((int)correctedColor.G) + ", " + std::to_string((int)correctedColor.B));
            }
            neoPixelStrip->SetPixelColor(ledMap(i), correctedColor);
        }
        neoPixelStrip->Show();
    }

#endif
}

uint8_t LedStripComponent::getDitheredBrightness(uint8_t brightness, uint8_t frame) // frame goes 0-7
{

    static const uint8_t ditherTable[8][8] = {
        {0, 32, 8, 40, 2, 34, 10, 42},
        {48, 16, 56, 24, 50, 18, 58, 26},
        {12, 44, 4, 36, 14, 46, 6, 38},
        {60, 28, 52, 20, 62, 30, 54, 22},
        {3, 35, 11, 43, 1, 33, 9, 41},
        {51, 19, 59, 27, 49, 17, 57, 25},
        {15, 47, 7, 39, 13, 45, 5, 37},
        {63, 31, 55, 23, 61, 29, 53, 21}};

    uint8_t threshold = ditherTable[frame & 0x07][brightness & 0x07];
    return (brightness > threshold) ? brightness : 0;
}

int LedStripComponent::ledMap(int index) const
{
    return (invertStrip ? count - 1 - index : index);
}
