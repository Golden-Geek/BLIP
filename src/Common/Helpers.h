#define DSTR(x) #x
#define XSTR(x) DSTR(x)
#define COMMA ,

// #define DBG(t)
// #define NDBG(t)
#define DBG(text) CommunicationComponent::instance->sendDebug(text)
#define NDBG(text) CommunicationComponent::instance->sendDebug(text, name)

#define DeviceID SettingsComponent::instance->getDeviceID()
#define DeviceType SettingsComponent::instance->deviceType
#define DeviceName SettingsComponent::instance->deviceName

// Class Helpers
#define DeclareSingleton(Class) static Class *instance;
#define ImplementSingleton(Class) Class *Class::instance = NULL;
#define ImplementManagerSingleton(Class) Class##ManagerComponent *Class##ManagerComponent::instance = NULL;
#define InitSingleton() instance = this;
#define DeleteSingleton() instance = NULL;

// Event Helpers
#define DeclareEventTypes(...) enum EventTypes \
{                                              \
    __VA_ARGS__,                               \
    TYPES_MAX                                  \
};

// Component Helpers
#define AddComponent(comp, name, Type, enabled, index) comp = addComponent<Type##Component>(name, enabled, o["components"][name], index);
#define AddOwnedComponent(comp) addComponent(comp, o["components"][(comp)->name]);
#define AddDefaultComponentListener(comp) comp->addListener(std::bind(&Component::onChildComponentEvent, this, std::placeholders::_1));

// > Component Class definition
#define PDerive(Class) , public Class
#define DeclareComponentClass(ParentClass, ClassPrefix, ...) \
    class ClassPrefix##Component : public ParentClass        \
                                       __VA_ARGS__           \
    {                                                        \
    public:

#define DeclareComponentSingletonEnabled(ClassPrefix, Type, Derives, Enabled)                                                             \
    DeclareComponentClass(Component, ClassPrefix, Derives)                                                                                \
        DeclareSingleton(ClassPrefix##Component)                                                                                          \
            ClassPrefix##Component(const std::string &name = Type, bool enabled = Enabled) : Component(name, enabled) { InitSingleton() } \
    ~ClassPrefix##Component() { DeleteSingleton() }                                                                                       \
    virtual std::string getTypeString() const override { return Type; }

#define DeclareComponentSingleton(ClassPrefix, Type, Derives)                                                                          \
    DeclareComponentClass(Component, ClassPrefix, Derives)                                                                             \
        DeclareSingleton(ClassPrefix##Component)                                                                                       \
            ClassPrefix##Component(const std::string &name = Type, bool enabled = true) : Component(name, enabled) { InitSingleton() } \
    ~ClassPrefix##Component() { DeleteSingleton() }                                                                                    \
    virtual std::string getTypeString() const override { return Type; }

#define DeclareComponentWithPriority(ClassPrefix, Type, Priority, Derives)                                                           \
    DeclareComponentClass(Component, ClassPrefix, Derives)                                                                           \
        ClassPrefix##Component(const std::string &name = Type, bool enabled = true, int index = 0) : Component(name, enabled, index) \
    {                                                                                                                                \
        isHighPriority = Priority;                                                                                                   \
    }                                                                                                                                \
    ~ClassPrefix##Component() {}                                                                                                     \
    virtual std::string getTypeString() const override { return Type; }

#define DeclareComponent(ClassPrefix, Type, Derives) DeclareComponentWithPriority(ClassPrefix, Type, false, Derives)
#define DeclareHighPriorityComponent(ClassPrefix, Type, Derives) DeclareComponentWithPriority(ClassPrefix, Type, true, Derives)

#define EndDeclareComponent \
    }                       \
    ;

// Manager

#ifdef MANAGER_USE_STATIC_ITEMS
#define DefineStaticItems(Type, MType) Type##Component items[MType##_MAX_COUNT];
#define AddStaticOrDynamicComponent(name, Type, enabled, index) \
    items[i].name = name;                                       \
    items[i].enabled = enabled;                                 \
    items[i].index = index;                                     \
    AddOwnedComponent(&items[i]);
#else
#define DefineStaticItems(Type, MType) Type##Component *items[MType##_MAX_COUNT];
#define AddStaticOrDynamicComponent(name, Type, enabled, index) AddComponent(items[i], name, Type, enabled, index);
#endif

#define DeclareComponentManagerCount(Type, MType, mName, itemName, MaxCount, DefaultCount, Fixed) \
    DeclareComponentSingleton(Type##Manager, #mName, )                                            \
        DeclareIntParam(count, DefaultCount);                                                     \
                                                                                                  \
    DefineStaticItems(Type, MType);                                                               \
    void setupInternal(JsonObject o) override                                                     \
    {                                                                                             \
        if (!Fixed)                                                                               \
            AddIntParamConfig(count);                                                             \
        for (int i = 0; i < count && i < MaxCount; i++)                                           \
        {                                                                                         \
            std::string n = #itemName + std::to_string(i + 1);                                    \
            AddStaticOrDynamicComponent(n, Type, i == 0, i);                                      \
            addItemInternal(i);                                                                   \
        }                                                                                         \
    }                                                                                             \
    virtual void addItemInternal(int index) {}

#define DeclareComponentMaybeFixedManager(Type, MType, mName, itemName, MaxCount, Fixed) \
    DeclareComponentManagerCount(Type, MType, mName, itemName, MaxCount, 1, Fixed)

#define DeclareComponentManager(Type, MType, mName, itemName, MaxCount) \
    DeclareComponentManagerCount(Type, MType, mName, itemName, MaxCount, 1, false)

// > Events
#define DeclareComponentEventTypes(...) enum ComponentEventTypes \
{                                                                \
    __VA_ARGS__,                                                 \
    TYPES_MAX                                                    \
};
#define DeclareComponentEventNames(...)                            \
    const std::string componentEventNames[TYPES_MAX]{__VA_ARGS__}; \
    std::string getComponentEventName(uint8_t type) const override { return componentEventNames[type]; }

// Command Helpers
#define CheckCommand(cmd, num) checkCommand(command, cmd, numData, num)
#define CommandCheck(cmd, Count)                                                               \
    if (command == cmd)                                                                        \
    {                                                                                          \
        if (numData < Count)                                                                   \
        {                                                                                      \
            NDBG("setConfig needs 2 parameters, only " + std::string(numData) + " provided."); \
            return false;                                                                      \
        }                                                                                      \
        else                                                                                   \
        {
#define ElifCommandCheck(cmd, Count) EndCommandCheck else CommandCheck(cmd, Count)
#define EndCommandCheck \
    }                   \
    }

// Parameters

// Class-less parameter system
#define DeclareBoolParam(name, val) bool name = val;
#define DeclareIntParam(name, val) int name = val;
#define DeclareEnumParam(name, val) int name = val;
#define DeclareFloatParam(name, val) float name = val;
#define DeclareRangeParam(name, val, minRange, maxRange) \
    float name = val;                                    \
    ParamRange name##Range{minRange, maxRange};
#define DeclareStringParam(name, val) std::string name = val;
#define DeclareP2DParam(name, val1, val2) float name[2]{val1, val2};
#define DeclareP2DRangeParam(name, minX, minY, maxX, maxY) DeclareP2DParam(name, (minX + maxX) / 2, (minY + maxY) / 2) \
    ParamRange name##Range{minX, maxX, minY, maxY};
#define DeclareP3DParam(name, val1, val2, val3) float name[3]{val1, val2, val3};
#define DeclareP3DRangeParam(name, minX, minY, minZ, maxX, maxY, maxZ) DeclareP3DParam(name, (minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2) \
    ParamRange name##Range{minX, maxX, minY, maxY, minZ, maxZ};

#define DeclareColorParam(name, r, g, b, a) float name[4]{r, g, b, a};

#define AddFunctionTrigger(func) addTrigger(#func, [this]() { this->func(); });

#define AddParamWithTag(type, class, param, tag)    \
    addParam(&param, ParamType::type, #param, tag); \
    SetParam(param, Settings::getVal<class>(o, #param, param));

#define AddParamWithRangeTag(type, class, param, tag)                        \
    addParam(&param, ParamType::type, #param, tag)->setRange(&param##Range); \
    SetParam(param, Settings::getVal<class>(o, #param, param));

#define AddParamWithOptionsTag(type, class, param, tag, options, numOptions)         \
    addParam(&param, ParamType::type, #param, tag)->setOptions(options, numOptions); \
    SetParam(param, Settings::getVal<class>(o, #param, param));

#define AddMultiParamWithTag(type, class, param, tag, numData, ExtraSet) \
    {                                                                    \
                                                                         \
        addParam(&param, ParamType::type, #param, tag) ExtraSet;         \
        if (o.containsKey(#param))                                       \
        {                                                                \
            JsonArray vArr = o[#param].as<JsonArray>();                  \
            var dataV[numData];                                          \
            dataV[0] = vArr[0].as<class>();                              \
            dataV[1] = vArr[1].as<class>();                              \
            if (numData > 2)                                             \
                dataV[2] = vArr[2].as<class>();                          \
            setParam(param, dataV, numData);                             \
        }                                                                \
    }

#define EmptyExtra ;

#define AddBoolParamWithTag(param, tag) AddParamWithTag(Bool, bool, param, tag)
#define AddIntParamWithTag(param, tag) AddParamWithTag(Int, int, param, tag)
#define AddFloatParamWithTag(param, tag) AddParamWithTag(Float, float, param, tag)
#define AddRangeParamWithTag(param, tag) AddFloatParamWithTag(param, tag)
#define AddStringParamWithTag(param, tag) AddParamWithTag(Str, std::string, param, tag)
#define AddP2DParamWithTag(param, tag) AddMultiParamWithTag(P2D, float, param, tag, 2, EmptyExtra)
#define AddP2DRangeParamWithTag(param, tag) AddMultiParamWithTag(P2D, float, param, tag, 2, ->setRange(&param##Range))
#define AddP3DParamWithTag(param, tag) AddMultiParamWithTag(P3D, float, param, tag, 3, EmptyExtra)
#define AddP3DRangeParamWithTag(param, tag) AddMultiParamWithTag(P3D, float, param, tag, 3, ->setRange(&param##Range))
#define AddColorParamWithTag(param, tag) AddMultiParamWithTag(TypeColor, float, param, tag, 4, EmptyExtra)
#define AddEnumParamWithTag(param, tag, options, numOptions) AddParamWithOptionsTag(TypeEnum, int, param, tag, options, numOptions)

#define AddBoolParam(param) AddBoolParamWithTag(param, TagNone)
#define AddIntParam(param) AddIntParamWithTag(param, TagNone)
#define AddFloatParam(param) AddFloatParamWithTag(param, TagNone)
#define AddRangeParam(param) AddRangeParamWithTag(param, TagNone)
#define AddStringParam(param) AddStringParamWithTag(param, TagNone)
#define AddP2DParam(param) AddP2DParamWithTag(param, TagNone)
#define AddP2DRangeParam(param) AddP2DRangeParamWithTag(param, TagNone)
#define AddP3DParam(param) AddP3DParamWithTag(param, TagNone)
#define AddP3DRangeParam(param) AddP3DRangeParamWithTag(param, TagNone)
#define AddColorParam(param) AddColorParamWithTag(param, TagNone)
#define AddEnumParam(param, options, numOptions) AddEnumParamWithTag(param, TagNone, options, numOptions)

#define AddBoolParamConfig(param) AddBoolParamWithTag(param, TagConfig)
#define AddIntParamConfig(param) AddIntParamWithTag(param, TagConfig)
#define AddFloatParamConfig(param) AddFloatParamWithTag(param, TagConfig)
#define AddRangeParamConfig(param) AddRangeParamWithTag(param, TagConfig)
#define AddStringParamConfig(param) AddStringParamWithTag(param, TagConfig)
#define AddP2DParamConfig(param) AddP2DParamWithTag(param, TagConfig)
#define AddP2DRangeParamConfig(param) AddP2DRangeParamWithTag(param, TagConfig)
#define AddP3DParamConfig(param) AddP3DParamWithTag(param, TagConfig)
#define AddP3DRangeParamConfig(param) AddP3DRangeParamWithTag(param, TagConfig)
#define AddColorParamConfig(param) AddColorParamWithTag(param, TagConfig)
#define AddEnumParamConfig(param, options, numOptions) AddEnumParamWithTag(param, TagConfig, options, numOptions)

#define AddBoolParamFeedback(param) AddBoolParamWithTag(param, TagFeedback)
#define AddIntParamFeedback(param) AddIntParamWithTag(param, TagFeedback)
#define AddFloatParamFeedback(param) AddFloatParamWithTag(param, TagFeedback)
#define AddRangeParamFeedback(param) AddRangeParamWithTag(param, TagFeedback)
#define AddStringParamFeedback(param) AddStringParamWithTag(param, TagFeedback)
#define AddP2DParamFeedback(param) AddP2DParamWithTag(param, TagFeedback)
#define AddP2DRangeParamFeedback(param) AddP2DRangeParamWithTag(param, TagFeedback)
#define AddP3DParamFeedback(param) AddP3DParamWithTag(param, TagFeedback)
#define AddP3DRangeParamFeedback(param) AddP3DRangeParamWithTag(param, TagFeedback)
#define AddColorParamFeedback(param) AddColorParamWithTag(param, TagFeedback)
#define AddEnumParamFeedback(param, options, numOptions) AddEnumParamWithTag(param, TagFeedback, options, numOptions)

#define SetParam(param, val)        \
    {                               \
        var pData[1];               \
        pData[0] = val;             \
        setParam(&param, pData, 1); \
    };
#define SetParam2(param, val1, val2) \
    {                                \
        var pData[2];                \
        pData[0] = val1;             \
        pData[1] = val2;             \
        setParam(&param, pData, 2);  \
    };
#define SetParam3(param, val1, val2, val3) \
    {                                      \
        var pData[3];                      \
        pData[0] = val1;                   \
        pData[1] = val2;                   \
        pData[2] = val3;                   \
        setParam(&param, pData, 3);        \
    };

// Script

#define LinkScriptFunctionsStart                                                           \
    virtual void linkScriptFunctionsInternal(IM3Module module, const char *tName) override \
    {

#define LinkScriptFunctionsStartMotherClass(Class) Class::linkScriptFunctionsInternal(module, tName);

#define LinkScriptFunctionsEnd }

#define LinkScriptFunction(Class, FunctionName, ReturnType, Args) m3_LinkRawFunctionEx(module, tName, XSTR(FunctionName), XSTR(ReturnType(Args)), &Class::m3_##FunctionName, this);

#define DeclareScriptFunctionVoid(Class, FunctionName, CallArgs, DeclarationArgs, GetArgs) \
    static m3ApiRawFunction(m3_##FunctionName)                                             \
    {                                                                                      \
        GetArgs;                                                                           \
        static_cast<Class *>(_ctx->userdata)->FunctionName##FromScript(CallArgs);          \
        m3ApiSuccess();                                                                    \
    }                                                                                      \
    virtual void FunctionName##FromScript(DeclarationArgs)

#define DeclareScriptFunctionReturn(Class, FunctionName, ReturnType, CallArgs, DeclarationArgs, GetArgs) \
    static m3ApiRawFunction(m3_##FunctionName)                                                           \
    {                                                                                                    \
        m3ApiReturnType(ReturnType);                                                                     \
        GetArgs;                                                                                         \
        ReturnType result = static_cast<Class *>(_ctx->userdata)->FunctionName##FromScript(CallArgs);    \
        m3ApiReturn(result);                                                                             \
    }                                                                                                    \
    virtual ReturnType FunctionName##FromScript(DeclarationArgs)

#define SA(Type, index) m3ApiGetArg(Type, arg##index);
#define CA(Type, index) Type arg##index

#define DeclareScriptFunctionVoid0(Class, FunctionName) DeclareScriptFunctionVoid(Class, FunctionName, , , )
#define DeclareScriptFunctionVoid1(Class, FunctionName, Type1) DeclareScriptFunctionVoid(Class, FunctionName, arg1, CA(Type1, 1), SA(Type1, 1))
#define DeclareScriptFunctionVoid2(Class, FunctionName, Type1, Type2) DeclareScriptFunctionVoid(Class, FunctionName, arg1 COMMA arg2, CA(Type1, 1) COMMA CA(Type2, 2), SA(Type1, 1) SA(Type2, 2))
#define DeclareScriptFunctionVoid3(Class, FunctionName, Type1, Type2, Type3) DeclareScriptFunctionVoid(Class, FunctionName, arg1 COMMA arg2 COMMA arg3, CA(Type1, 1) COMMA CA(Type2, 2) COMMA CA(Type3, 3), SA(Type1, 1) SA(Type2, 2) SA(Type3, 3))
#define DeclareScriptFunctionVoid4(Class, FunctionName, Type1, Type2, Type3, Type4) DeclareScriptFunctionVoid(Class, FunctionName, arg1 COMMA arg2 COMMA arg3 COMMA arg4, CA(Type1, 1) COMMA CA(Type2, 2) COMMA CA(Type3, 3) COMMA CA(Type4, 4), SA(Type1, 1) SA(Type2, 2) SA(Type3, 3) SA(Type4, 4))
#define DeclareScriptFunctionVoid5(Class, FunctionName, Type1, Type2, Type3, Type4, Type5) DeclareScriptFunctionVoid(Class, FunctionName, arg1 COMMA arg2 COMMA arg3 COMMA arg4 COMMA arg5, CA(Type1, 1) COMMA CA(Type2, 2) COMMA CA(Type3, 3) COMMA CA(Type4, 4) COMMA CA(Type5, 5), SA(Type1, 1) SA(Type2, 2) SA(Type3, 3) SA(Type5, 4) SA(Type5, 5))
#define DeclareScriptFunctionVoid6(Class, FunctionName, Type1, Type2, Type3, Type4, Type5, Type6) DeclareScriptFunctionVoid(Class, FunctionName, arg1 COMMA arg2 COMMA arg3 COMMA arg4 COMMA arg5 COMMA arg6, CA(Type1, 1) COMMA CA(Type2, 2) COMMA CA(Type3, 3) COMMA CA(Type4, 4) COMMA CA(Type5, 5) COMMA CA(Type6, 6), SA(Type1, 1) SA(Type2, 2) SA(Type3, 3) SA(Type4, 4) SA(Type5, 5) SA(Type6, 6))

#define DeclareScriptFunctionReturn0(Class, FunctionName, ReturnType) DeclareScriptFunctionReturn(Class, FunctionName, ReturnType, , , )
#define DeclareScriptFunctionReturn1(Class, FunctionName, ReturnType, Type1) DeclareScriptFunctionReturn(Class, FunctionName, ReturnType, arg1, CA(Type1, 1), SA(Type1, 1))
#define DeclareScriptFunctionReturn2(Class, FunctionName, ReturnType, Type1, Type2) DeclareScriptFunctionReturn(Class, FunctionName, ReturnType, arg1 COMMA arg2, CA(Type1, 1) COMMA CA(Type2, 2), SA(Type1, 1) SA(Type2, 2))
#define DeclareScriptFunctionReturn3(Class, FunctionName, ReturnType, Type1, Type2, Type3) DeclareScriptFunctionReturn(Class, FunctionName, ReturnType, arg1 COMMA arg2 COMMA arg3, CA(Type1, 1) COMMA CA(Type2, 2) COMMA CA(Type3, 3), SA(Type1, 1) SA(Type2, 2) SA(Type3, 3))
