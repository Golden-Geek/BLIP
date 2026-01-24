#pragma once

#define MAX_CHILD_COMPONENTS 16
#define MAX_CHILD_PARAMS 32

class Component : public EventBroadcaster<ComponentEvent>
{
public:
    Component(const String &name, bool _enabled = true, int index = 0) : name(name),
                                                                         enabled(_enabled),
                                                                         index(index),
                                                                         isInit(false),
                                                                         autoInit(true),
                                                                         exposeEnabled(true),
                                                                         saveEnabled(true),
                                                                         lastUpdateTime(0),
                                                                         lastFeedbackTime(0),
                                                                         parentComponent(NULL),
                                                                         numComponents(0),
                                                                         numParams(0)
    {
    }

    virtual ~Component() {}
    virtual String getTypeString() const { return "[notype]"; }

    String name;
    int index; // used for multiple instances of the same component type
    bool autoInit;
    bool isInit;
    bool exposeEnabled;
    bool saveEnabled;

    long lastUpdateTime;
    long lastFeedbackTime;

    DeclareBoolParam(enabled, true);
    DeclareIntParam(updateRate, 0);
    DeclareFloatParam(feedbackRate, 0);

    Component *parentComponent;

    Component *components[MAX_CHILD_COMPONENTS];
    uint8_t numComponents;

    // Parameter *parameters[MAX_CHILD_PARAMETERS];
    // uint8_t numParameters;

    enum ParamType
    {
        Trigger,
        Bool,
        Int,
        Float,
        Str,
        P2D,
        P3D,
        TypeColor,
        TypeEnum,
        ParamTypeMax
    };

    enum ParamTag
    {
        TagNone = 0,
        TagConfig = 1,
        TagFeedback = 2,
        TagNameMax
    };

    struct ParamRange
    {
        float vMin = 0;
        float vMax = 0;
        float vMin2 = 0;
        float vMax2 = 0;
        float vMin3 = 0;
        float vMax3 = 0;
    };

    const String typeNames[ParamTypeMax]{"I", "b", "i", "f", "s", "ff", "fff", "r", "i"};
    const String tagNames[TagNameMax]{"", "config", "feedback"};

    void *params[MAX_CHILD_PARAMS];
    std::map<void *, ParamType> paramTypesMap;
    std::map<void *, String> paramToNameMap;
    std::map<String, void *> nameToParamMap;
    std::map<void *, uint8_t> paramTagsMap;
    std::map<void *, String *> enumOptionsMap;
    std::map<void *, int> enumOptionsCountMap;
    std::map<void *, ParamRange> paramRangesMap;
    std::map<String, std::function<void(void)>> triggersMap;

    uint8_t numParams;

    virtual String getComponentEventName(uint8_t type) const
    {
        return "[noname]";
    }

    virtual void onChildComponentEvent(const ComponentEvent &e) {}

    void setup(JsonObject o = JsonObject());
    bool init();
    virtual void update();
    void clear();

    virtual void setupInternal(JsonObject o) {}
    virtual bool initInternal() { return true; }
    virtual void updateInternal() {}
    virtual void clearInternal() {}

    void setCustomUpdateRate(int defaultRate, JsonObject o)
    {
        if (updateRate < 0)
            return;
        updateRate = defaultRate;
        AddIntParamConfig(updateRate);
    }

    void setCustomFeedbackRate(float defaultRate, JsonObject o)
    {
        if (feedbackRate < 0)
            return;
        feedbackRate = defaultRate;
        AddFloatParamConfig(feedbackRate);
    }

    void sendEvent(uint8_t type, var *data = NULL, int numData = 0)
    {
        EventBroadcaster::sendEvent(ComponentEvent(this, type, data, numData));
    }

    template <class T>
    T *addComponent(const String &name, bool _enabled, JsonObject o = JsonObject(), int index = 0) { return (T *)addComponent(new T(name, _enabled, index), o); };

    Component *addComponent(Component *c, JsonObject o = JsonObject());

    Component *getComponentWithName(const String &name);

    void addParam(void *param, ParamType type, const String &name, uint8_t tags = TagNone);
    void setParamRange(void *param, ParamRange range);
    void setEnumOptions(void *param, String *enumOptions, int numOptions);
    void setParam(void *param, var *value, int numData);
    ParamType getParamType(void *param) const;
    bool checkParamTag(void *param, ParamTag tag) const;
    String getParamString(void *param) const;

    void addTrigger(const String &name, std::function<void(void)> func);

     void setParamTag(void *param, ParamTag tag, bool enable)
    {
        uint8_t currentTags = paramTagsMap[param];
        if (enable)
            currentTags |= tag;
        else
            currentTags &= ~tag;
        paramTagsMap[param] = currentTags;
    }

    void setParamConfig(void *param, bool config) { setParamTag(param, TagConfig, config); }
    void setParamFeedback(void *param, bool feedback) { setParamTag(param, TagFeedback, feedback); }
    
    void setEnabled(bool v) { SetParam(enabled, v); }
    virtual void onEnabledChanged() {}

    void paramValueChanged(void *param);
    virtual void paramValueChangedInternal(void *param) {}
    virtual void childParamValueChanged(Component *caller, Component *comp, void *param);
    virtual bool checkParamsFeedback(void *param);

    virtual String getEnumString(void *param) const { return ""; }

    bool handleCommand(const String &command, var *data, int numData);
    virtual bool handleCommandInternal(const String &command, var *data, int numData) { return false; }
    bool checkCommand(const String &command, const String &ref, int numData, int expectedData);

    bool handleSetParam(const String &paramName, var *data, int numData);

    void fillSettingsData(JsonObject o);
    void fillSettingsParam(JsonObject o, const String &pName, void *param);
    virtual void fillOSCQueryParam(JsonObject o, const String &fullPath, void *param, bool showConfig = true);

    enum OSCQueryChunkType
    {
        Start,
        Content,
        End,
        ChunkTypeMax
    };

    struct OSCQueryChunk
    {
        OSCQueryChunk(Component *c, OSCQueryChunkType t = Start, String d = "") : nextComponent(c), nextType(t), data(d) {}
        Component *nextComponent = nullptr;
        OSCQueryChunkType nextType = Start;
        String data = "";
    };

    void fillChunkedOSCQueryData(OSCQueryChunk *chunk, bool showConfig = true);
    void setupChunkAfterComponent(OSCQueryChunk *result, const Component *c);

    String getFullPath(bool includeRoot = false, bool scriptMode = false, bool serialMode = false) const;

// void scripting
#ifdef USE_SCRIPT
    virtual void linkScriptFunctions(IM3Module module, bool isLocal = false);
    virtual void linkScriptFunctionsInternal(IM3Module module, const char *tName) {}

    DeclareScriptFunctionVoid1(Component, setEnabled, int) { SetParam(enabled, arg1); }
#endif
};