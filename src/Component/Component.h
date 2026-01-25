#pragma once

class Component : public EventBroadcaster<ComponentEvent>
{
public:
    Component(const std::string &name, bool _enabled = true, int index = 0) : name(name),
                                                                         enabled(_enabled),
                                                                         index(index),
                                                                         isInit(false),
                                                                         autoInit(true),
                                                                         exposeEnabled(true),
                                                                         saveEnabled(true),
                                                                         isHighPriority(false),
                                                                         lastUpdateTime(0),
                                                                         lastFeedbackTime(0),
                                                                         parentComponent(nullptr)
    {
    }

    virtual ~Component() {}
    virtual std::string getTypeString() const { return "[notype]"; }

    std::string name;
    int index; // used for multiple instances of the same component type
    bool autoInit;
    bool isInit;
    bool exposeEnabled;
    bool saveEnabled;
    bool isHighPriority;

    long lastUpdateTime;
    long lastFeedbackTime;

    DeclareBoolParam(enabled, true);
    DeclareIntParam(updateRate, 0);
    DeclareFloatParam(feedbackRate, 0);

    Component *parentComponent;

    std::vector<Component *> components;

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

    const std::string typeNames[ParamTypeMax]{"I", "b", "i", "f", "s", "ff", "fff", "r", "i"};
    const std::string tagNames[TagNameMax]{"", "config", "feedback"};

    std::vector<void *> params;
    std::map<void *, int> paramTypesMap;
    std::map<void *, std::string> paramToNameMap;
    std::map<std::string, void *> nameToParamMap;
    std::map<void *, uint8_t> paramTagsMap;
    std::map<void *, const std::string *> enumOptionsMap;
    std::map<void *, int> enumOptionsCountMap;
    std::map<void *, ParamRange> paramRangesMap;
    std::map<std::string, std::function<void(void)>> triggersMap;

    virtual std::string getComponentEventName(uint8_t type) const
    {
        return "[noname]";
    }

    virtual void onChildComponentEvent(const ComponentEvent &e) {}

    void setup(JsonObject o = JsonObject());
    bool init();
    virtual void update(bool inFastLoop = false);
    void clear();

    virtual void setupInternal(JsonObject o) {}
    virtual bool initInternal() { return true; }
    virtual void updateInternal() {}
    virtual void clearInternal() {}

    void setCustomUpdateRate(int defaultRate, JsonObject o);
    void setCustomFeedbackRate(float defaultRate, JsonObject o);
    void sendEvent(uint8_t type, var *data = NULL, int numData = 0);

    template <class T>
    T *addComponent(const std::string &name, bool _enabled, JsonObject o = JsonObject(), int index = 0) { return (T *)addComponent(new T(name, _enabled, index), o); };
    Component *addComponent(Component *c, JsonObject o = JsonObject());
    // Component *getComponentWithName(const std::string &name);

    void addParam(void *param, ParamType type, const std::string &name, uint8_t tags = TagNone);
    void setParamRange(void *param, ParamRange range);
    void setEnumOptions(void *param, const std::string *enumOptions, int numOptions);
    void setParam(void *param, var *value, int numData);
    ParamType getParamType(void *param) const;
    bool checkParamTag(void *param, ParamTag tag) const;
    void addTrigger(const std::string &name, std::function<void(void)> func);

    void setParamTag(void *param, ParamTag tag, bool enable);
    void setParamConfig(void *param, bool config) { setParamTag(param, TagConfig, config); }
    void setParamFeedback(void *param, bool feedback) { setParamTag(param, TagFeedback, feedback); }

    void setEnabled(bool v) { SetParam(enabled, v); }
    virtual void onEnabledChanged() {}

    void paramValueChanged(void *param);
    virtual void paramValueChangedInternal(void *param) {}
    virtual void childParamValueChanged(Component *caller, Component *comp, void *param);
    virtual bool checkParamsFeedback(void *param);

    bool handleCommand(const std::string &command, var *data, int numData);
    virtual bool handleCommandInternal(const std::string &command, var *data, int numData) { return false; }
    bool checkCommand(const std::string &command, const std::string &ref, int numData, int expectedData);
    bool handleSetParam(const std::string &paramName, var *data, int numData);

    void fillSettingsData(JsonObject o);
    void fillSettingsParam(JsonObject o, void *param);
    bool fillOSCQueryParam(JsonObject o, const std::string &fullPath, void *param, bool showConfig = true);
    JsonObject createBaseOSCQueryObject(JsonObject o, const std::string &fullPath, const std::string &pName, const std::string &type, bool readOnly);

    enum OSCQueryChunkType
    {
        Start,
        Content,
        End,
        ChunkTypeMax
    };

    struct OSCQueryChunk
    {
        OSCQueryChunk(Component *c, OSCQueryChunkType t = Start, std::string d = "") : nextComponent(c), nextType(t), data(d) {}
        Component *nextComponent = nullptr;
        OSCQueryChunkType nextType = Start;
        std::string data = "";
    };

    void fillChunkedOSCQueryData(OSCQueryChunk *chunk, bool showConfig = true);
    void setupChunkAfterComponent(OSCQueryChunk *result, const Component *c);

    std::string getFullPath(bool includeRoot = false, bool scriptMode = false, bool serialMode = false) const;

// void scripting
#ifdef USE_SCRIPT
    virtual void linkScriptFunctions(IM3Module module, bool isLocal = false);
    virtual void linkScriptFunctionsInternal(IM3Module module, const char *tName) {}

    DeclareScriptFunctionVoid1(Component, setEnabled, int) { SetParam(enabled, arg1); }
#endif
};