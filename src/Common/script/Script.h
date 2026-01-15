#pragma once

#ifndef SCRIPT_MAX_SIZE
#define SCRIPT_MAX_SIZE 16000
#endif

#ifndef WASM_STACK_SLOTS
#define WASM_STACK_SLOTS 2000
#endif

#ifndef SCRIPT_NATIVE_STACK_SIZE
#define SCRIPT_NATIVE_STACK_SIZE (16 * 1024)
#endif

#ifndef WASM_MEMORY_LIMIT
#define WASM_MEMORY_LIMIT 4096
#endif

#ifndef WASM_ASYNC
#define WASM_ASYNC 0
#endif

#ifndef WASM_VARIABLES_MAX
#define WASM_VARIABLES_MAX 20
#endif

#ifndef WASM_FUNCTIONS_MAX
#define WASM_FUNCTIONS_MAX 20
#endif

#ifndef WASM_EVENTS_MAX
#define WASM_EVENTS_MAX 20
#endif

class ScriptComponent;

class Script
{
public:
    Script();
    ~Script() {}

    bool isRunning;
    unsigned char scriptData[SCRIPT_MAX_SIZE];

    // struct ScriptVariable
    // {
    // String name;
    // String niceName;
    // float min;
    // float max;
    // };

    String variableNames[WASM_VARIABLES_MAX];
    float mins[WASM_VARIABLES_MAX];
    float maxs[WASM_VARIABLES_MAX];
    int variableCount;

    String functionNames[WASM_FUNCTIONS_MAX];
    int functionCount;

    String eventNames[WASM_EVENTS_MAX];
    int eventCount;

    long scriptSize;

    ScriptComponent *localComponent;

    IM3Runtime runtime;
    IM3Environment env;

    IM3Function initFunc;
    IM3Function updateFunc;
    IM3Function stopFunc;
    IM3Function setScriptParamFunc;
    IM3Function triggerFunctionFunc;

    bool isInUpdateFunc;
    static float timeAtLaunch;

    void load(const String &name);

    long tstart;
    long tend;

    bool init();
    void update();
    void launchWasm();
    void shutdown();
    void stop();

    void logWasm(String funcName, M3Result r);

    void setScriptParam(String paramName, float value);
    void triggerFunction(String funcName);

    void sendScriptEvent(int eventId);
    void sendScriptParamFeedback(int paramId, float value);

    void setParamsFromDMX(const uint8_t *data, uint16_t len);

#if WASM_ASYNC
    static void launchWasmTaskStatic(void *);
#endif
    void launchWasmTask();

    M3Result LinkArduino(IM3Runtime runtime);
};