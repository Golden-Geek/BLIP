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

    DeclareBoolParam(enabled, true);

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
        TagNone,
        TagConfig,
        TagNameMax
    };

    const String typeNames[ParamTypeMax]{"I", "b", "i", "f", "s", "ff", "fff", "r", "i"};
    const String tagNames[TagNameMax]{"", "config"};

    void *params[MAX_CHILD_PARAMS];
    ParamType paramTypes[MAX_CHILD_PARAMS];
    ParamTag paramTags[MAX_CHILD_PARAMS];

    uint8_t numParams;

    virtual String getComponentEventName(uint8_t type) const
    {
        return "[noname]";
    }

    virtual void onChildComponentEvent(const ComponentEvent &e) {}

    void setup(JsonObject o = JsonObject());
    bool init();
    void update();
    void clear();

    virtual void setupInternal(JsonObject o) {}
    virtual bool initInternal() { return true; }
    virtual void updateInternal() {}
    virtual void clearInternal() {}

    void sendEvent(uint8_t type, var *data = NULL, int numData = 0)
    {
        EventBroadcaster::sendEvent(ComponentEvent(this, type, data, numData));
    }

    template <class T>
    T *addComponent(const String &name, bool _enabled, JsonObject o = JsonObject(), int index = 0) { return (T *)addComponent(new T(name, _enabled, index), o); };

    Component *addComponent(Component *c, JsonObject o = JsonObject());

    Component *getComponentWithName(const String &name);

    void addParam(void *param, ParamType type, ParamTag tag = TagNone);
    void setParam(void *param, var *value, int nmData);
    ParamType getParamType(void *param) const;
    ParamTag getParamTag(void *param) const;
    String getParamString(void *param) const;

    void toggleEnabled() { SetParam(enabled, !enabled); }
    virtual void onEnabledChanged() {}

    void paramValueChanged(void *param);
    virtual void paramValueChangedInternal(void *param) {}
    virtual void childParamValueChanged(Component *caller, Component *comp, void *param);
    virtual bool checkParamsFeedback(void *param);
    virtual bool checkParamsFeedbackInternal(void *param) { return false; }
    // virtual void sendParamFeedback(void* param);

    virtual String getEnumString(void *param) const { return ""; }

    bool handleCommand(const String &command, var *data, int numData);
    virtual bool handleCommandInternal(const String &command, var *data, int numData) { return false; }
    bool checkCommand(const String &command, const String &ref, int numData, int expectedData);

    bool handleSetParam(const String &paramName, var *data, int numData);
    virtual bool handleSetParamInternal(const String &paramName, var *data, int numData) { return false; }

    void fillSettingsData(JsonObject o, bool showConfig = true);
    virtual void fillSettingsParamsInternal(JsonObject o, bool showConfig = true) {}

    // virtual void fillOSCQueryData(JsonObject o, bool includeConfig = true, bool recursive = true);
    virtual void fillOSCQueryParamsInternal(JsonObject o, const String &fullPath, bool showConfig = true) {}
    virtual void fillOSCQueryParam(JsonObject o, const String &fullPath, const String &pName, ParamType t, void *param,
                                   bool showConfig = true, bool readOnly = false, const String *options = nullptr, int numOptions = 0,
                                   float vMin = 0, float vMax = 0, float vMin2 = 0, float vMax2 = 0, float vMin3 = 0, float vMax3 = 0);

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

    String getFullPath(bool includeRoot = false, bool scriptMode = false) const;

// void scripting
#ifdef USE_SCRIPT
    virtual void linkScriptFunctions(IM3Module module, bool isLocal = false);
    virtual void linkScriptFunctionsInternal(IM3Module module, const char *tName) {}

    DeclareScriptFunctionVoid1(Component, setEnabled, int) { SetParam(enabled, arg1); }
#endif
};