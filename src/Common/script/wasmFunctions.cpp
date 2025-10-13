#ifdef USE_SCRIPT
#include "UnityIncludes.h"

SimplexNoise sn;

m3ApiRawFunction(m3_arduino_millis)
{
    m3ApiReturnType(uint32_t);

    m3ApiReturn(millis());
}

m3ApiRawFunction(m3_arduino_getTime)
{
    m3ApiReturnType(float);
    float v = millis() / 1000.0f - ScriptComponent::instance->script.timeAtLaunch;
    m3ApiReturn(v);
}

m3ApiRawFunction(m3_arduino_delay)
{
    m3ApiGetArg(uint32_t, ms);
    delay(ms);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_printFloat)
{
    m3ApiGetArg(float, val);
    DBG("Print from script : " + String(val));
    m3ApiSuccess();
}

m3ApiRawFunction(m3_printInt)
{
    m3ApiGetArg(uint32_t, val);
    DBG("Print from script : " + String(val));
    m3ApiSuccess();
}

#ifdef USE_LEDSTRIP
m3ApiRawFunction(m3_clearLeds)
{
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.clearColors();

    m3ApiSuccess();
}

m3ApiRawFunction(m3_dimLeds)
{
    m3ApiGetArg(float, v);
    // RootComponent::instance->strips.items[0]->scriptLayer.dimLayer(v);

    m3ApiSuccess();
}

m3ApiRawFunction(m3_fillLeds)
{
    m3ApiGetArg(uint32_t, color);
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.fillAll(Color(color));
    m3ApiSuccess();
}

m3ApiRawFunction(m3_fillLedsRGB)
{
    m3ApiGetArg(uint32_t, r);
    m3ApiGetArg(uint32_t, g);
    m3ApiGetArg(uint32_t, b);
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.fillAll(Color((uint8_t)r, (uint8_t)g, (uint8_t)b));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_fillLedsHSV)
{
    m3ApiGetArg(float, h);
    m3ApiGetArg(float, s);
    m3ApiGetArg(float, v);
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.fillAll(Color::HSV(h, s, v));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_setLed)
{
    m3ApiGetArg(uint32_t, index);
    m3ApiGetArg(uint32_t, color);
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.setLed(index, Color(color));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_getLed)
{
    m3ApiReturnType(uint32_t)
        m3ApiGetArg(uint32_t, index);

    if (index < LED_MAX_COUNT)
    {
        Color c = RootComponent::instance->strips.items[0]->streamLayer.colors[index];
        uint32_t val = c.r << 16 | c.g << 8 | c.b;
        m3ApiReturn(val);
    }

    m3ApiReturn(0)
}

m3ApiRawFunction(m3_setLedRGB)
{
    m3ApiGetArg(uint32_t, index);
    m3ApiGetArg(uint32_t, r);
    m3ApiGetArg(uint32_t, g);
    m3ApiGetArg(uint32_t, b);
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.setLed(index, Color((uint8_t)r, (uint8_t)g, (uint8_t)b));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_setLedHSV)
{
    m3ApiGetArg(uint32_t, index);
    m3ApiGetArg(float, h);
    m3ApiGetArg(float, s);
    m3ApiGetArg(float, v);
    // RootComponent::instance->strips.items[0]->bakeLayer(LedManager::Mode::Stream);
    RootComponent::instance->strips.items[0]->scriptLayer.setLed(index, Color::HSV(h, s, v));

    m3ApiSuccess();
}

m3ApiRawFunction(m3_pointRGB)
{
    m3ApiGetArg(float, pos);
    m3ApiGetArg(float, radius);
    m3ApiGetArg(uint32_t, r);
    m3ApiGetArg(uint32_t, g);
    m3ApiGetArg(uint32_t, b);

    RootComponent::instance->strips.items[0]->scriptLayer.point(Color((uint8_t)r, (uint8_t)g, (uint8_t)b), pos, radius, false);

    m3ApiSuccess();
}

m3ApiRawFunction(m3_pointHSV)
{
    m3ApiGetArg(float, pos);
    m3ApiGetArg(float, radius);
    m3ApiGetArg(float, h);
    m3ApiGetArg(float, s);
    m3ApiGetArg(float, v);
    RootComponent::instance->strips.items[0]->scriptLayer.point(Color::HSV(h, s, v), pos, radius, false);

    m3ApiSuccess();
}

m3ApiRawFunction(m3_setIR)
{
    m3ApiGetArg(float, v);
    // RootComponent::instance->strips.items[0]->bakeLayer.setBrightness(v);

    m3ApiSuccess();
}

m3ApiRawFunction(m3_updateLeds)
{
    RootComponent::instance->strips.items[0]->scriptLayer.update();
    m3ApiSuccess();
}

m3ApiRawFunction(m3_getFXSpeed)
{
    m3ApiReturnType(float);
    float v = 0; // RootComponent::instance->strips.items[0]->bakeLayer.offsetSpeed;
    m3ApiReturn((float)v);
}

m3ApiRawFunction(m3_getFXIsoSpeed)
{
    m3ApiReturnType(float);
    float v = 0; // RootComponent::instance->strips.items[0]->bakeLayer.isolationSpeed;
    m3ApiReturn((float)v);
}

m3ApiRawFunction(m3_getFXStaticOffset)
{
    m3ApiReturnType(float);
    float v = 0; // RootComponent::instance->strips.items[0]->bakeLayer.staticOffset;
    m3ApiReturn((float)v);
}

m3ApiRawFunction(m3_getFXFlipped)
{
    m3ApiReturnType(uint32_t);
    bool v = false; // RootComponent::instance->strips.items[0]->bakeLayer.boardIsFlipped;
    m3ApiReturn((uint32_t)v);
}

m3ApiRawFunction(m3_setFXSpeed)
{
    m3ApiGetArg(float, sp);
    // RootComponent::instance->strips.items[0]->bakeLayer.offsetSpeed = sp;
    m3ApiSuccess();
}

m3ApiRawFunction(m3_setFXIsoSpeed)
{
    m3ApiGetArg(float, sp);
    // RootComponent::instance->strips.items[0]->bakeLayer.isolationSpeed = sp;
    m3ApiSuccess();
}

m3ApiRawFunction(m3_setFXIsoAxis)
{
    m3ApiGetArg(uint32_t, ax);
    // RootComponent::instance->strips.items[0]->bakeLayer.isolationAxis = ax;
    m3ApiSuccess();
}

m3ApiRawFunction(m3_setFXStaticOffset)
{
    m3ApiGetArg(float, sp);
    // RootComponent::instance->strips.items[0]->bakeLayer.staticOffset = sp;
    m3ApiSuccess();
}

m3ApiRawFunction(m3_resetFX)
{
    // RootComponent::instance->strips.items[0]->bakeLayer.reset();
    m3ApiSuccess();
}

#endif

#ifdef USE_BATTERY
m3ApiRawFunction(m3_setBatterySendEnabled)
{
    m3ApiGetArg(uint32_t, en);
    RootComponent::instance->battery.feedbackInterval = en ? 5 : 0;
    m3ApiSuccess();
}
#endif

#ifdef USE_FILES
m3ApiRawFunction(m3_playVariant)
{
    m3ApiGetArg(uint32_t, v);
    String name = RootComponent::instance->strips.items[0]->bakeLayer.curFile.name();
    float time = RootComponent::instance->strips.items[0]->bakeLayer.curTimeMs;
    uint32_t start = millis();
    char l = name.charAt(name.length() - 1);

    if (l >= '0' && l <= '9')
    {
        name.remove(name.length() - 1);
    }

    name = name + String(v);

    RootComponent::instance->strips.items[0]->bakeLayer.load(name);

    time = (time + (millis() - start)) / 1000;
    RootComponent::instance->strips.items[0]->bakeLayer.play(time);
    m3ApiSuccess();
}
#endif

#ifdef USE_MOTION
m3ApiRawFunction(m3_getOrientation)
{
    m3ApiReturnType(float);
    m3ApiGetArg(uint32_t, oi);

    float v = oi < 3 ? RootComponent::instance->motion.orientation[oi] : -1;

    m3ApiReturn(v);
}

m3ApiRawFunction(m3_getYaw)
{
    m3ApiReturnType(float);
    m3ApiReturn(RootComponent::instance->motion.orientation[0]);
}

m3ApiRawFunction(m3_getPitch)
{
    m3ApiReturnType(float);
    m3ApiReturn(RootComponent::instance->motion.orientation[1]);
}
m3ApiRawFunction(m3_getRoll)
{
    m3ApiReturnType(float);
    m3ApiReturn(RootComponent::instance->motion.orientation[2]);
}

m3ApiRawFunction(m3_getThrowState)
{
    m3ApiReturnType(uint32_t);
    m3ApiReturn((uint32_t)RootComponent::instance->motion.throwState);
}

m3ApiRawFunction(m3_getProjectedAngle)
{
    m3ApiReturnType(float);
    m3ApiReturn(RootComponent::instance->motion.projectedAngle);
}

m3ApiRawFunction(m3_setProjectedAngleOffset)
{
    m3ApiGetArg(float, yaw);
    m3ApiGetArg(float, angle);
    // RootComponent::instance->motion.setProjectAngleOffset(yaw, angle);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_calibrateIMU)
{
    // RootComponent::instance->motion.calibrate();
    m3ApiSuccess();
}

m3ApiRawFunction(m3_setIMUEnabled)
{
    m3ApiGetArg(uint32_t, en);
    RootComponent::instance->motion.setEnabledFromScript((bool)en);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_getActivity)
{
    m3ApiReturnType(float);
    m3ApiReturn(RootComponent::instance->motion.activity);
}

m3ApiRawFunction(m3_getSpin)
{
    m3ApiReturnType(float);
    m3ApiReturn(RootComponent::instance->motion.spin);
}
#endif

#ifdef USE_BUTTON
m3ApiRawFunction(m3_getButtonState)
{
    m3ApiReturnType(uint32_t);

    m3ApiGetArg(uint32_t, btIndex);

    int v = btIndex < RootComponent::instance->buttons.count ? RootComponent::instance->buttons.items[btIndex]->value : 0;

    m3ApiReturn((uint32_t)v);
}
#endif

#ifdef USE_MIC
m3ApiRawFunction(m3_setMicEnabled)
{
    m3ApiGetArg(uint32_t, en);
    MicManager::instance->setEnabled((bool)en);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_getMicLevel)
{
    m3ApiReturnType(float);

    float v = MicManager::instance->enveloppe;
    m3ApiReturn((float)v);
}
#endif

#ifdef USE_DISTANCE
m3ApiRawFunction(m3_getDistance)
{
    m3ApiReturnType(float);

    m3ApiGetArg(uint32_t, sensorIndex);

    float v = sensorIndex < RootComponent::instance->distances.count ? RootComponent::instance->distances.items[sensorIndex]->value : -1.0f;

    m3ApiReturn((float)v);
}
#endif

m3ApiRawFunction(m3_randomInt)
{
    m3ApiReturnType(uint32_t);
    m3ApiGetArg(uint32_t, min);
    m3ApiGetArg(uint32_t, max);

    m3ApiReturn((uint32_t)random(min, max + 1));
}

m3ApiRawFunction(m3_noise)
{
    m3ApiReturnType(float);
    m3ApiGetArg(float, x);
    m3ApiGetArg(float, y);

    float n = (float)sn.noise(x, y);
    n = (n + 1) / 2; // convert to value range [0..1]

    m3ApiReturn(n);
}

#endif