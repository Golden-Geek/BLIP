#include "UnityIncludes.h"

#define FATAL(func, msg)                  \
    {                                     \
        Serial.print("Fatal: " func " "); \
        Serial.println(msg);              \
        return;                           \
    }

float Script::timeAtLaunch = 0.f;

Script::Script(Component *localComponent) : isRunning(false),
                                            localComponent(localComponent),
                                            runtime(NULL),
                                            // env(NULL),
                                            initFunc(NULL),
                                            updateFunc(NULL),
                                            stopFunc(NULL)
{
    DBG("Script Init here");
}

bool Script::init()
{
    DBG("Script init.");
    env = m3_NewEnvironment();

    if (!env)
    {
        DBG("Script environment error");
        return false;
    }

    return true;
}

void Script::update()
{
    if (isRunning)
    {
        // TSTART()
        if (updateFunc != NULL)
        {
            m3_CallV(updateFunc);
        }
        // TFINISH("Script ")
    }
}

void Script::load(const String &path)
{
    if (isRunning)
    {
        DBG("Script is running, stop before load");
        stop();
    }

    DBG("Load script " + path + "...");

#ifdef USE_FILES
    File f = FilesComponent::instance->openFile("/scripts/" + path + ".wasm", false); // false is for reading
    if (!f)
    {
        DBG("Error reading file " + path);
        return;
    }

    long totalBytes = f.size();
    if (totalBytes > SCRIPT_MAX_SIZE)
    {
        DBG("Script size is more than max size");
        return;
    }
    scriptSize = totalBytes;

    f.read(scriptData, scriptSize);

    DBG("Script read " + String(scriptSize) + " bytes");
    launchWasm();
#else
    DBG("Script loading not supported, USE_FILES not defined");
#endif
}

void Script::launchWasm()
{
    DBG("Script Launching wasm...");
    if (isRunning)
        stop();

#if WASM_ASYNC
    xTaskCreate(&Script::launchWasmTaskStatic, "wasm3", SCRIPT_NATIVE_STACK_SIZE, this, 5, NULL);
    DBG("Wasm task launched");
#else
    launchWasmTask();
#endif
}

#if WASM_ASYNC
void Script::launchWasmTaskStatic(void *v)
{
    ((Script *)v)->launchWasmTask();
}
#endif

void Script::launchWasmTask()
{
    M3Result result = m3Err_none;

    if (runtime != NULL)
    {
        DBG("New run free Runtime");
        m3_FreeRuntime(runtime);
    }

    runtime = m3_NewRuntime(env, WASM_STACK_SLOTS, NULL);
    if (!runtime)
    {
        DBG("Script runtime setup error");
        return;
    }

#ifdef WASM_MEMORY_LIMIT
    runtime->memoryLimit = WASM_MEMORY_LIMIT;
#endif

    IM3Module module;
    result = m3_ParseModule(env, &module, scriptData, scriptSize);
    if (result)
        FATAL("ParseModule", result);

    result = m3_LoadModule(runtime, module);
    if (result)
        FATAL("LoadModule", result);

    result = LinkArduino(runtime);
    if (result)
        FATAL("LinkArduino", result);

    DBG("Finding functions");
    String foundFunc;
    result = m3_FindFunction(&initFunc, runtime, "init");
    if (initFunc != NULL)
        foundFunc += "init";
    result = m3_FindFunction(&updateFunc, runtime, "update");
    if (updateFunc != NULL)
        foundFunc += " / update";

    result = m3_FindFunction(&stopFunc, runtime, "stop");
    if (stopFunc != NULL)
        foundFunc += " / stop";

    result = m3_FindFunction(&setScriptParamFunc, runtime, "setParam");
    if (setScriptParamFunc != NULL)
        foundFunc += " / setParam";

    DBG("Found functions : " + foundFunc);

    isRunning = true;

    timeAtLaunch = millis() / 1000.0f;

    if (initFunc != NULL)
    {
        DBG("Calling init");
        result = m3_CallV(initFunc);
    }

    Serial.println("Running WebAssembly...");

#if WASM_ASYNC
    vTaskDelete(NULL);
#endif
}

M3Result Script::LinkArduino(IM3Runtime runtime)
{
    IM3Module module = runtime->modules;
    const char *arduino = "arduino";

    m3_LinkRawFunction(module, arduino, "millis", "i()", &m3_arduino_millis);
    m3_LinkRawFunction(module, arduino, "getTime", "f()", &m3_arduino_getTime);
    m3_LinkRawFunction(module, arduino, "delay", "v(i)", &m3_arduino_delay);
    m3_LinkRawFunction(module, arduino, "printFloat", "v(f)", &m3_printFloat);
    m3_LinkRawFunction(module, arduino, "printInt", "v(i)", &m3_printInt);

#ifdef USE_LEDSTRIP
    m3_LinkRawFunction(module, arduino, "clearLeds", "v()", &m3_clearLeds);
    m3_LinkRawFunction(module, arduino, "dimLeds", "v(f)", &m3_dimLeds);
    m3_LinkRawFunction(module, arduino, "fillLeds", "v(i)", &m3_fillLeds);
    m3_LinkRawFunction(module, arduino, "fillLedsRGB", "v(iii)", &m3_fillLedsRGB);
    m3_LinkRawFunction(module, arduino, "fillLedsHSV", "v(fff)", &m3_fillLedsHSV);
    m3_LinkRawFunction(module, arduino, "setLed", "v(ii)", &m3_setLed);
    m3_LinkRawFunction(module, arduino, "getLed", "i(i)", &m3_getLed);

    m3_LinkRawFunction(module, arduino, "setLedRGB", "v(iiii)", &m3_setLedRGB);
    m3_LinkRawFunction(module, arduino, "setLedHSV", "v(ifff)", &m3_setLedHSV);
    m3_LinkRawFunction(module, arduino, "pointRGB", "v(ffiii)", &m3_pointRGB);
    m3_LinkRawFunction(module, arduino, "pointHSV", "v(fffff)", &m3_pointHSV);
    m3_LinkRawFunction(module, arduino, "setIR", "v(f)", &m3_setIR);
    m3_LinkRawFunction(module, arduino, "playVariant", "v(i)", &m3_playVariant);
    m3_LinkRawFunction(module, arduino, "updateLeds", "v()", &m3_updateLeds);

    m3_LinkRawFunction(module, arduino, "getFXSpeed", "f()", &m3_getFXSpeed);
    m3_LinkRawFunction(module, arduino, "getFXIsoSpeed", "f()", &m3_getFXIsoSpeed);
    m3_LinkRawFunction(module, arduino, "getFXStaticOffset", "f()", &m3_getFXStaticOffset);
    m3_LinkRawFunction(module, arduino, "getFXFlipped", "i()", &m3_getFXFlipped);
    m3_LinkRawFunction(module, arduino, "setFXSpeed", "v(f)", &m3_setFXSpeed);
    m3_LinkRawFunction(module, arduino, "setFXIsoSpeed", "v(f)", &m3_setFXIsoSpeed);
    m3_LinkRawFunction(module, arduino, "setFXIsoAxis", "v(i)", &m3_setFXIsoAxis);
    m3_LinkRawFunction(module, arduino, "setFXStaticOffset", "v(f)", &m3_setFXStaticOffset);
    m3_LinkRawFunction(module, arduino, "resetFX", "v()", &m3_resetFX);
#endif

#ifdef USE_MOTION
    m3_LinkRawFunction(module, arduino, "getOrientation", "f(i)", &m3_getOrientation);
    m3_LinkRawFunction(module, arduino, "getYaw", "f()", &m3_getYaw);
    m3_LinkRawFunction(module, arduino, "getPitch", "f()", &m3_getPitch);
    m3_LinkRawFunction(module, arduino, "getRoll", "f()", &m3_getRoll);
    m3_LinkRawFunction(module, arduino, "getProjectedAngle", "f()", &m3_getProjectedAngle);
    m3_LinkRawFunction(module, arduino, "setProjectedAngleOffset", "v(ff)", &m3_setProjectedAngleOffset);
    m3_LinkRawFunction(module, arduino, "setIMUEnabled", "v(i)", &m3_setIMUEnabled);
    m3_LinkRawFunction(module, arduino, "calibrateIMU", "v()", &m3_calibrateIMU);
    m3_LinkRawFunction(module, arduino, "getThrowState", "i()", &m3_getThrowState);
    m3_LinkRawFunction(module, arduino, "getButtonState", "i(i)", &m3_getButtonState);
    m3_LinkRawFunction(module, arduino, "getActivity", "f()", &m3_getActivity);
    m3_LinkRawFunction(module, arduino, "getSpin", "f()", &m3_getSpin);
#endif

#ifdef USE_MIC
    m3_LinkRawFunction(module, arduino, "setMicEnabled", "v(i)", &m3_setMicEnabled);
    m3_LinkRawFunction(module, arduino, "getMicLevel", "f()", &m3_getMicLevel);
#endif

#ifdef USE_BATTERY
    m3_LinkRawFunction(module, arduino, "setBatterySendEnabled", "v(i)", &m3_setBatterySendEnabled);
#endif

    m3_LinkRawFunction(module, arduino, "randomInt", "i(ii)", &m3_randomInt);
    m3_LinkRawFunction(module, arduino, "noise", "f(ff)", &m3_noise);

    if (localComponent != NULL)
        localComponent->linkScriptFunctions(module, true);

    RootComponent::instance->linkScriptFunctions(module);

    return m3Err_none;
}

void Script::stop()
{
    if (isRunning)
    {
        DBG("Stopping script");

        if (stopFunc != NULL)
            m3_CallV(stopFunc);

        isRunning = false;
        m3_FreeRuntime(runtime);
        runtime = NULL;
    }
    else
    {
        DBG("Not stopping script, because non was running");
    }
}

void Script::setScriptParam(int index, float value)
{
    if (stopFunc != NULL)
        m3_CallV(setScriptParamFunc, index, value);
}

void Script::shutdown()
{
    stop();
    m3_FreeEnvironment(env);
    env = NULL;
}
