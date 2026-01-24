#include "UnityIncludes.h"

#define SetupFastLED(Type, DataPin, count) FastLED.addLeds<Type, DataPin>(leds, count)

ImplementManagerSingleton(LedStrip);

void LedStripComponent::setupInternal(JsonObject o)
{
// init
#ifdef LED_USE_FASTLED
#else
    neoPixelStrip = NULL;
    dotStarStrip = NULL;
#endif

    for (int i = 0; i < LEDSTRIP_NUM_USER_LAYERS; i++)
        userLayers[i] = NULL;

    if (index == 1)
    {
        dataPin = LED2_DEFAULT_DATA_PIN;
        clkPin = LED2_DEFAULT_CLK_PIN;
        enPin = LED2_DEFAULT_EN_PIN;
    }

    AddIntParam(count);
    AddIntParamConfig(dataPin);
    AddIntParamConfig(enPin);
    AddIntParamConfig(clkPin);
    AddFloatParam(brightness);
    AddBoolParamConfig(invertStrip);
    AddIntParam(multiLedMode);
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
        // NDBG("Setting Led Enable pin : " + String(enPin));
        pinMode(enPin, OUTPUT);
        digitalWrite(enPin, HIGH); // enable LEDs
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
        NDBG("Using FastLED with DotStar strip on pin " + String(LED_DEFAULT_DATA_PIN) + " and clk pin " + String(LED_DEFAULT_CLK_PIN));
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

    if (clkPin > 0)
    {
        NDBG("Using DotStar strip");
        dotStarStrip = new Adafruit_DotStar(count * LED_DUPLICATE, dataPin, clkPin, DOTSTAR_BGR);
        dotStarStrip->begin();
    }
    else
    {
        NDBG("Using NeoPixel strip");

        neoPixelStrip = new Adafruit_NeoPixel(count * LED_DUPLICATE, dataPin, LED_DEFAULT_COLOR_ORDER + NEO_KHZ800);
        neoPixelStrip->begin();
    }
#endif

    setStripPower(true);
    showLeds(); // may fail when using files
}

void LedStripComponent::updateInternal()
{
#ifdef LED_USE_FASTLED
#else
    if (dotStarStrip == NULL && neoPixelStrip == NULL)
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

    delete dotStarStrip;
    dotStarStrip = NULL;
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
        if (neoPixelStrip != NULL)
            neoPixelStrip->updateLength(count);
        else if (dotStarStrip != NULL)
            dotStarStrip->updateLength(count);
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
        NDBG("Set Strip Power " + String(value));
        digitalWrite(enPin, value); // enable LEDs
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

    // DBG("Processing Layer "+String(layer->name));

    for (int i = 0; i < count; i++)
    {
        int index = getColorIndex(i);
        Color c = layer->colors[index];
        // if(index == 0)
        // {
        //     DBG(" Layer color for led " + String(i) + ": " + String(c.r) + ", " + String(c.g) + ", " + String(c.b) + ", " + String(c.a));
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
    // DBG("Showing Leds, led 0 color: " + String(colors[0].r) + ", " + String(colors[0].g) + ", " + String(colors[0].b) + ", " + String(colors[0].a));
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

        // NDBG("Total Amps at full brightness: " + String(totalAmpFullBrightness) + ", Max Power: " + String(maxPower));

        if (totalAmpFullBrightness > maxPower)
        {
            float maxOkBrightness = LED_BRIGHTNESS_FACTOR * maxPower / (float)totalAmpFullBrightness;
            targetBrightness = min(targetBrightness, maxOkBrightness);
            // NDBG("maxOkBrightness: " + String(maxOkBrightness) + ", Target brightness: " + String(targetBrightness));
        }
    }

    // DBG("Strip power is " + String(currentStripPower) + ", brightness : " + String(brightness) + ", target brightness: " + String(targetBrightness));

#ifdef LED_USE_FASTLED
    FastLED.setBrightness(min(int(targetBrightness * 255), 255));
    for (int i = 0; i < count; i++)
    {
        float a = colors[i].a / 255.0f;
        leds[ledMap(i)] = CRGB(colors[i].r * a, colors[i].g * a, colors[i].b * a);
    }
    // DBG("First Led " + String(ledMap(0)) + " color: " + String(colors[0].r) + ", " + String(colors[0].g) + ", " + String(colors[0].b) + ", " + String(colors[0].a));
    FastLED.show();
#else
    if (neoPixelStrip != NULL)
        neoPixelStrip->setBrightness(targetBrightness * 255);
    else if (dotStarStrip != NULL)
        dotStarStrip->setBrightness(targetBrightness * 255);

    if (neoPixelStrip != NULL)
    {
        for (int i = 0; i < count; i++)
        {

            float a = colors[i].a / 255.0f;
            uint8_t r = Adafruit_NeoPixel::gamma8(colors[i].r * a);
            uint8_t g = Adafruit_NeoPixel::gamma8(colors[i].g * a);
            uint8_t b = Adafruit_NeoPixel::gamma8(colors[i].b * a);
            neoPixelStrip->setPixelColor(ledMap(i), r, g, b);
        }
        neoPixelStrip->show();
    }
    else if (dotStarStrip != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            float a = colors[i].a / 255.0f;
            // float bFactor = getDitheredBrightness(targetBrightness * colors[i].a, ditherFrameCounter) / 255.0f;
            uint8_t r = Adafruit_NeoPixel::gamma8(colors[i].r * a);
            uint8_t g = Adafruit_NeoPixel::gamma8(colors[i].g * a);
            uint8_t b = Adafruit_NeoPixel::gamma8(colors[i].b * a);
            dotStarStrip->setPixelColor(ledMap(i), r, g, b);
        }
        dotStarStrip->show();
    }
    ditherFrameCounter = (ditherFrameCounter + 1) & 0x07;
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
