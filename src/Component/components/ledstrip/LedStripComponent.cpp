ImplementManagerSingleton(LedStrip);

void LedStripComponent::setupInternal(JsonObject o)
{
    // init
    neoPixelStrip = NULL;
    dotStarStrip = NULL;

    for (int i = 0; i < LEDSTRIP_NUM_USER_LAYERS; i++)
        userLayers[i] = NULL;

    AddIntParam(count);
    AddIntParamConfig(dataPin);
    AddIntParamConfig(enPin);
    AddIntParamConfig(clkPin);
    AddFloatParam(brightness);
    AddBoolParamConfig(invertStrip);
    AddIntParamConfig(maxPower);

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
    for (int i = 0; i < count; i++)
        colors[i] = Color(0, 0, 0, 0);

    if (clkPin > 0)
    {
        NDBG("Using DotStar strip");
        dotStarStrip = new Adafruit_DotStar(count, dataPin, clkPin, DOTSTAR_BGR);
        dotStarStrip->begin();
    }
    else
    {
        NDBG("Using NeoPixel strip");
        neoPixelStrip = new Adafruit_NeoPixel(count, dataPin, NEO_GRB + NEO_KHZ800);
        neoPixelStrip->begin();
    }

    setStripPower(true);
}

void LedStripComponent::updateInternal()
{

    if (dotStarStrip == NULL && neoPixelStrip == NULL)
    {
        return; // not active
    }

    // all layer's internal colors are updated in Component's update() function

    clearColors();
    for (int i = 0; i < LEDSTRIP_NUM_USER_LAYERS; i++)
        processLayer(userLayers[i]);

#if USE_SYSTEMLAYER
    processLayer(&systemLayer);
#endif

    showLeds();
}

void LedStripComponent::clearInternal()
{
    clearColors();
    showLeds();
    setStripPower(false);

    delete neoPixelStrip;
    neoPixelStrip = NULL;

    delete dotStarStrip;
    dotStarStrip = NULL;
}

void LedStripComponent::setBrightness(float val)
{
    SetParam(brightness, val);
}

void LedStripComponent::paramValueChangedInternal(void *param)
{
    if (param == &count)
    {
        if (neoPixelStrip != NULL)
            neoPixelStrip->updateLength(count);
        else if (dotStarStrip != NULL)
            dotStarStrip->updateLength(count);
    }
}

void LedStripComponent::onEnabledChanged()
{
    setStripPower(enabled);
}

void LedStripComponent::setStripPower(bool value)
{
    DBG("Set Strip Power " + String(value));
    if (enPin > 0)
        digitalWrite(enPin, value); // enable LEDs

    pinMode(dataPin, value ? OUTPUT : OPEN_DRAIN);
}

// Layer functions
void LedStripComponent::processLayer(LedStripLayer *layer)
{
    if (layer == NULL)
        return;

    if (!layer->enabled)
        return;

    for (int i = 0; i < count; i++)
    {
        Color c = layer->colors[i];

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

        default:
            break;
        }
    }

#if USE_FX
    fx.process(colors);
#endif
}

// Color functions

void LedStripComponent::clearColors()
{
    // for(int i=0;i<count;i++) colors[i] = Color(0,0,0,0);
    memset(colors, 0, sizeof(Color) * count);
}

void LedStripComponent::showLeds()
{
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

    if (neoPixelStrip != NULL)
        neoPixelStrip->setBrightness(targetBrightness * 255);
    else if (dotStarStrip != NULL)
        dotStarStrip->setBrightness(targetBrightness * 255);

    if (neoPixelStrip != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            float a = colors[i].a / 255.0f;
            // float bFactor = getDitheredBrightness(targetBrightness * , ditherFrameCounter) / 255.0f;
            neoPixelStrip->setPixelColor(ledMap(i),
                                         Adafruit_NeoPixel::gamma8(colors[i].r * a),
                                         Adafruit_NeoPixel::gamma8(colors[i].g * a),
                                         Adafruit_NeoPixel::gamma8(colors[i].b * a));
        }
        neoPixelStrip->show();
    }
    else if (dotStarStrip != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            float a = colors[i].a / 255.0f;
            // float bFactor = getDitheredBrightness(targetBrightness * colors[i].a, ditherFrameCounter) / 255.0f;
            dotStarStrip->setPixelColor(ledMap(i),
                                        Adafruit_DotStar::gamma8(colors[i].r * a),
                                        Adafruit_DotStar::gamma8(colors[i].g * a),
                                        Adafruit_DotStar::gamma8(colors[i].b * a));
        }
        dotStarStrip->show();
    }

    ditherFrameCounter = (ditherFrameCounter + 1) & 0x07;
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
