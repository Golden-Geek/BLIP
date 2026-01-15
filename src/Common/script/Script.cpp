#include "UnityIncludes.h"

#define FATAL(func, msg)                  \
    {                                     \
        Serial.print("Fatal: " func " "); \
        Serial.println(msg);              \
        return;                           \
    }

float Script::timeAtLaunch = 0.f;

Script::Script() : localComponent(NULL),
                   isRunning(false),
                   runtime(NULL),
                   variableCount(0),
                   functionCount(0),
                   eventCount(0),
                   // env(NULL),
                   initFunc(NULL),
                   updateFunc(NULL),
                   stopFunc(NULL),
                   isInUpdateFunc(false)
{
}

bool Script::init()
{
    env = m3_NewEnvironment();

    if (!env)
    {
        DBG("[script] !Script environment error");
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
            isInUpdateFunc = true;
            M3Result r = m3_CallV(updateFunc);
            isInUpdateFunc = false;
            logWasm("update", r);
        }
        // TFINISH("Script ")
    }
}

void Script::load(const String &path)
{
    if (isRunning)
    {
        DBG("[script] Script is running, stop before load");
        stop();
    }

    DBG("[script] Load script " + path + "...");

#ifdef USE_FILES
    File f = FilesComponent::instance->openFile("/scripts/" + path + ".wasm", false); // false is for reading
    if (!f)
    {
        DBG("[script] !Error reading file " + path);
        return;
    }

    StaticJsonDocument<1024> metaDataDoc;
    File mf = FilesComponent::instance->openFile("/scripts/" + path + "_metadata.wmeta", false); // false is for reading
    if (mf)
    {
        DeserializationError error = deserializeJson(metaDataDoc, mf);
        if (!error)
        {
            JsonObject obj = metaDataDoc.as<JsonObject>();
            JsonArray varNames = obj["variables"].as<JsonArray>();
            variableCount = 0;
            for (JsonVariant v : varNames)
            {
                if (variableCount >= WASM_VARIABLES_MAX)
                    break;
                JsonObject varObj = v.as<JsonObject>();
                variableNames[variableCount] = varObj["name"].as<String>();
                if (varObj.containsKey("min"))
                    mins[variableCount] = varObj["min"].as<float>();
                else
                    mins[variableCount] = INT32_MIN;
                if (varObj.containsKey("max"))
                    maxs[variableCount] = varObj["max"].as<float>();
                else
                    maxs[variableCount] = INT32_MAX;

                variableCount++;
            }

            DBG("[script] Loaded " + String(variableCount) + " variable names from metadata");

            JsonArray funcNames = obj["functions"].as<JsonArray>();
            functionCount = 0;
            for (JsonVariant v : funcNames)
            {
                if (functionCount >= WASM_FUNCTIONS_MAX)
                    break;

                JsonObject varObj = v.as<JsonObject>();
                functionNames[functionCount] = varObj["name"].as<String>();
                functionCount++;
            }

            DBG("[script] Loaded " + String(functionCount) + " function names from metadata");

            JsonArray evNames = obj["events"].as<JsonArray>();
            eventCount = 0;
            for (JsonVariant v : evNames)
            {
                if (eventCount >= WASM_EVENTS_MAX)
                    break;
                eventNames[eventCount] = v.as<String>();
                eventCount++;
            }
            DBG("[script] Loaded " + String(eventCount) + " event names from metadata");
        }
        else
        {
            DBG("[script] !Error parsing metadata json : " + String(error.c_str()));
        }
    }
    else
    {
        DBG("[script] No metadata file found for script " + path);
    }

    long totalBytes = f.size();
    if (totalBytes > SCRIPT_MAX_SIZE)
    {
        DBG("[script] !Script size is more than max size");
        return;
    }
    scriptSize = totalBytes;

    f.read(scriptData, scriptSize);

    DBG("[script] Script read " + String(scriptSize) + " bytes");
    launchWasm();
#else
    DBG("[script] Script loading not supported, USE_FILES not defined");
#endif
}

void Script::launchWasm()
{
    DBG("[script] Script Launching wasm...");
    if (isRunning)
        stop();

#if WASM_ASYNC
    xTaskCreate(&Script::launchWasmTaskStatic, "wasm3", SCRIPT_NATIVE_STACK_SIZE, this, 5, NULL);
    DBG("[script] Wasm task launched");
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
        DBG("[script] New run free Runtime");
        m3_FreeRuntime(runtime);
    }

    runtime = m3_NewRuntime(env, WASM_STACK_SLOTS, NULL);
    if (!runtime)
    {
        DBG("[script] !Script runtime setup error");
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

    DBG("[script] Finding functions");
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

    result = m3_FindFunction(&triggerFunctionFunc, runtime, "triggerFunction");
    if (triggerFunctionFunc != NULL)
        foundFunc += " / triggerFunction";

    DBG("[script] Found functions : " + foundFunc);

    isRunning = true;

    timeAtLaunch = millis() / 1000.0f;

    Serial.println("Running WebAssembly...");

    if (initFunc != NULL)
    {
        DBG("[script] Calling init");
        result = m3_CallV(initFunc);
        logWasm("init", result);
    }
    else
    {
        DBG("[script] No init function found");
    }

#if WASM_ASYNC
    vTaskDelete(NULL);
#endif
}

M3Result Script::LinkArduino(IM3Runtime runtime)
{
    IM3Module module = runtime->modules;
    m3_LinkRawFunction(module, "env", "abort", "v(iiii)", &m3_as_abort);

    const char *arduino = "arduino";

    m3_LinkRawFunction(module, arduino, "millis", "i()", &m3_arduino_millis);
    m3_LinkRawFunction(module, arduino, "getTime", "f()", &m3_arduino_getTime);
    m3_LinkRawFunction(module, arduino, "delay", "v(i)", &m3_arduino_delay);
    m3_LinkRawFunction(module, arduino, "printFloat", "v(f)", &m3_printFloat);
    m3_LinkRawFunction(module, arduino, "printInt", "v(i)", &m3_printInt);
    m3_LinkRawFunction(module, arduino, "printString", "v(ii)", &m3_printString);
    m3_LinkRawFunction(module, arduino, "sendEvent", "v(i)", &m3_sendEvent);
    m3_LinkRawFunction(module, arduino, "sendParamFeedback", "v(if)", &m3_sendParamFeedback);
    m3_LinkRawFunction(module, arduino, "getPropID", "i()", &m3_getPropID);

#ifdef USE_LEDSTRIP
    m3_LinkRawFunction(module, arduino, "getLedCount", "i()", &m3_getLedCount);
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
    m3_LinkRawFunction(module, arduino, "getActivity", "f()", &m3_getActivity);
    m3_LinkRawFunction(module, arduino, "getSpin", "f()", &m3_getSpin);
#endif

#ifdef USE_BUTTON
    m3_LinkRawFunction(module, arduino, "getButtonState", "i(i)", &m3_getButtonState);
#endif

#ifdef USE_IO
    m3_LinkRawFunction(module, arduino, "getIOValue", "f(i)", &m3_getIOValue);
    m3_LinkRawFunction(module, arduino, "setIOValue", "v(if)", &m3_setIOValue);
#endif

#ifdef USE_MIC
    m3_LinkRawFunction(module, arduino, "setMicEnabled", "v(i)", &m3_setMicEnabled);
    m3_LinkRawFunction(module, arduino, "getMicLevel", "f()", &m3_getMicLevel);
#endif

#ifdef USE_BATTERY
    m3_LinkRawFunction(module, arduino, "setBatterySendEnabled", "v(i)", &m3_setBatterySendEnabled);
#endif

#ifdef USE_DISTANCE
    m3_LinkRawFunction(module, arduino, "getDistance", "f(i)", &m3_getDistance);
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
        DBG("[script] Stopping script");

        if (stopFunc != NULL)
        {
            M3Result result = m3_CallV(stopFunc);
            logWasm("stop", result);
        }
        else
        {
            DBG("[script] No stop function found");
        }

        isRunning = false;
        m3_FreeRuntime(runtime);
        runtime = NULL;
    }
    else
    {
        DBG("[script] Not stopping script, because non was running");
    }
}

void Script::logWasm(String funcName, M3Result r)
{
    if (!r)
        return;
    M3ErrorInfo info;
    m3_GetErrorInfo(runtime, &info);
    DBG(String("[m3] Calling ") + funcName + " failed: " + info.message);
    if (info.file && info.function)
        DBG(String(" @ ") + String(info.file) + " :: " + String(info.function->names[0]) + " (line " + String(info.line) + ")");
}

void Script::setScriptParam(String paramName, float value)
{
    int paramIndex = -1;
    for (int i = 0; i < variableCount; i++)
    {
        if (variableNames[i] == paramName)
        {
            paramIndex = i;
            break;
        }
    }

    if (paramIndex != -1 && setScriptParamFunc != NULL)
    {
        // DBG("Setting script param " + paramName + " to value " + String(value));
        while (isInUpdateFunc)
            delay(1);
        m3_CallV(setScriptParamFunc, paramIndex, value);
    }
}

void Script::triggerFunction(String funcName)
{
    int funcIndex = -1;
    for (int i = 0; i < functionCount; i++)
    {
        if (functionNames[i] == funcName)
        {
            funcIndex = i;
            break;
        }
    }

    if (funcIndex != -1 && triggerFunctionFunc != NULL)
    {
        // DBG("Triggering script function " + funcName);
        while (isInUpdateFunc)
            delay(1);
        m3_CallV(triggerFunctionFunc, funcIndex);
    }
}

void Script::sendScriptEvent(int eventId)
{
    if (eventId < 0 || eventId >= eventCount)
        return;

    String eventName = eventNames[eventId];
    localComponent->sendScriptEvent(eventName);
}

void Script::sendScriptParamFeedback(int paramId, float value)
{
    if (paramId < 0 || paramId >= variableCount)
        return;

    String paramName = variableNames[paramId];

    localComponent->sendScriptParamFeedback(paramName, value);
}

void Script::setParamsFromDMX(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        if (i >= variableCount)
            break;

        float value = (float)data[i] / 255.0f; // normalize to 0..1
        if (mins[i] != INT32_MIN && maxs[i] != INT32_MAX)
            value = mins[i] + value * (maxs[i] - mins[i]);

        setScriptParam(variableNames[i], value);
    }
}

void Script::shutdown()
{
    stop();
    m3_FreeEnvironment(env);
    env = NULL;
}
