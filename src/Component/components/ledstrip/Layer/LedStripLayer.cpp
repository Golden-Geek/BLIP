#include "LedStripLayer.h"
LedStripLayer::LedStripLayer(const String &name, Type t, LedStripComponent *strip) : Component(name),
                                                                                     strip(strip),
                                                                                     type(t)
{
}

LedStripLayer::~LedStripLayer()
{
}

void LedStripLayer::setupInternal(JsonObject o)
{
    AddIntParam(blendMode);
    // blendMode.options = blendModeOptions;
    // blendMode.numOptions = BlendModeMax;

    memset(colors, 0, LED_MAX_COUNT * sizeof(Color));

    for (int i = 0; i < strip->numColors; i++)
        colors[i] = Color(0, 0, 0, 0);
}

// Helpers

void LedStripLayer::clearColors()
{
    // for(int i=0;i<count;i++) colors[i] = Color(0,0,0,0);
    memset(colors, 0, sizeof(Color) * strip->numColors);
}

void LedStripLayer::fillAll(Color c)
{
    fillRange(c, 0, 1);
}

void LedStripLayer::fillRange(Color c, float start, float end, bool doClear)
{
    if (doClear)
        clearColors();

    int s = max(min(start, end), 0.f) * (strip->numColors - 1);
    int e = min(max(start, end), 1.f) * (strip->numColors - 1);

    for (int i = s; i <= e; i++)
    {
        colors[i] += c;
    }
}

void LedStripLayer::point(Color c, float pos, float radius, bool doClear)
{
    if (doClear)
        clearColors();
    if (radius == 0)
        return;

    for (int i = 0; i < strip->numColors; i++)
    {
        float rel = i * 1.0f / max(strip->numColors - 1, 1);
        float fac = max(1 - (std::abs((float)(pos - rel)) / radius), 0.f);
        Color tc = c.withMultipliedAlpha(fac);
        colors[i] += tc;
    }
}
void LedStripLayer::setLed(int index, Color c)
{
    if (index >= 0 && index < strip->numColors)
        colors[index] = c;
}

Color LedStripLayer::getLed(int index)
{
    if (index >= 0 && index < strip->numColors)
        return colors[index];

    return Color(0, 0, 0, 0);
}

void LedStripLayer::setBlendMode(BlendMode b)
{
    SetParam(blendMode, b);
}
