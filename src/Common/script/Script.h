#pragma once

#ifndef SCRIPT_MAX_SIZE
#define SCRIPT_MAX_SIZE 16000
#endif

#ifndef WASM_STACK_SLOTS
#define WASM_STACK_SLOTS 1200
#endif

#ifndef SCRIPT_NATIVE_STACK_SIZE
#define SCRIPT_NATIVE_STACK_SIZE (8 * 1024)
#endif

#ifndef SCRIPT_MIN_STACK_WORDS
#define SCRIPT_MIN_STACK_WORDS 256
#endif

#ifndef WASM_MEMORY_LIMIT
#define WASM_MEMORY_LIMIT 2048
#endif

#ifndef WASM_VARIABLES_MAX
#define WASM_VARIABLES_MAX 10
#endif

#ifndef WASM_FUNCTIONS_MAX
#define WASM_FUNCTIONS_MAX 10
#endif

#ifndef WASM_EVENTS_MAX
#define WASM_EVENTS_MAX 4
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
    // std::string name;
    // std::string niceName;
    // float min;
    // float max;
    // };

    std::string variableNames[WASM_VARIABLES_MAX];
    float mins[WASM_VARIABLES_MAX];
    float maxs[WASM_VARIABLES_MAX];
    int variableCount;

    std::string functionNames[WASM_FUNCTIONS_MAX];
    int functionCount;

    std::string eventNames[WASM_EVENTS_MAX];
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

    void load(const std::string &name);

    long tstart;
    long tend;

    bool init();
    void update();
    void launchWasm();
    void shutdown();
    void stop();

    void logWasm(std::string funcName, M3Result r);

    void setScriptParam(std::string paramName, float value);
    void triggerFunction(std::string funcName);

    void sendScriptEvent(int eventId);
    void sendScriptParamFeedback(int paramId, float value);

    void setParamsFromDMX(const uint8_t *data, uint16_t len);

    void launchWasmTask();

    M3Result LinkArduino(IM3Runtime runtime);
};