#define STR(x) #x
#define XSTR(x) STR(x)
#define COMMA ,

// #define DBG(t)
// #define NDBG(t)
#define DBG(text) HWSerialComponent::instance->send(text)
#define NDBG(text) HWSerialComponent::instance->send("[" + name + "] " + text)

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

#define DeclareComponentSingletonEnabled(ClassPrefix, Type, Derives, Enabled)                                                        \
    DeclareComponentClass(Component, ClassPrefix, Derives)                                                                           \
        DeclareSingleton(ClassPrefix##Component)                                                                                     \
            ClassPrefix##Component(const String &name = Type, bool enabled = Enabled) : Component(name, enabled) { InitSingleton() } \
    ~ClassPrefix##Component() { DeleteSingleton() }                                                                                  \
    virtual String getTypeString() const override { return Type; }

#define DeclareComponentSingleton(ClassPrefix, Type, Derives)                                                                     \
    DeclareComponentClass(Component, ClassPrefix, Derives)                                                                        \
        DeclareSingleton(ClassPrefix##Component)                                                                                  \
            ClassPrefix##Component(const String &name = Type, bool enabled = true) : Component(name, enabled) { InitSingleton() } \
    ~ClassPrefix##Component() { DeleteSingleton() }                                                                               \
    virtual String getTypeString() const override { return Type; }

#define DeclareComponent(ClassPrefix, Type, Derives)                                                                               \
    DeclareComponentClass(Component, ClassPrefix, Derives)                                                                         \
        ClassPrefix##Component(const String &name = Type, bool enabled = true, int index = 0) : Component(name, enabled, index) {} \
    ~ClassPrefix##Component() {}                                                                                                   \
    virtual String getTypeString() const override { return Type; }

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

#define DeclareComponentManagerDefaultMax(Type, MType, mName, itemName) DeclareComponentManagerWithMax(Type, MType, mName, itemName, 8)


#define DeclareComponentManagerCount(Type, MType, mName, itemName, MaxCount, DefaultCount) \
    DeclareComponentSingleton(Type##Manager, #mName, )                                     \
        DeclareIntParam(count, DefaultCount);                                              \
                                                                                           \
    DefineStaticItems(Type, MType);                                                        \
    void setupInternal(JsonObject o) override                                              \
    {                                                                                      \
        AddIntParam(count);                                                                \
        for (int i = 0; i < count && i < MaxCount; i++)                                    \
        {                                                                                  \
            String n = #itemName + String(i + 1);                                          \
            AddStaticOrDynamicComponent(n, Type, i == 0, i);                               \
            addItemInternal(i);                                                            \
        }                                                                                  \
    }                                                                                      \
    virtual void addItemInternal(int index) {}                                             \
    HandleSetParamInternalStart                                                            \
        CheckAndSetParam(count);                                                           \
    HandleSetParamInternalEnd;                                                             \
    FillSettingsInternalStart                                                              \
        FillSettingsParam(count);                                                          \
    FillSettingsInternalEnd;                                                               \
    FillOSCQueryInternalStart                                                              \
        FillOSCQueryIntParam(count);                                                       \
    FillOSCQueryInternalEnd

    
#define DeclareComponentManager(Type, MType, mName, itemName, MaxCount) \
    DeclareComponentManagerCount(Type, MType, mName, itemName, MaxCount, 1)

// > Events
#define DeclareComponentEventTypes(...) enum ComponentEventTypes \
{                                                                \
    __VA_ARGS__,                                                 \
    TYPES_MAX                                                    \
};
#define DeclareComponentEventNames(...)                       \
    const String componentEventNames[TYPES_MAX]{__VA_ARGS__}; \
    String getComponentEventName(uint8_t type) const override { return componentEventNames[type]; }

// Command Helpers
#define CheckCommand(cmd, num) checkCommand(command, cmd, numData, num)
#define CommandCheck(cmd, Count)                                                          \
    if (command == cmd)                                                                   \
    {                                                                                     \
        if (numData < Count)                                                              \
        {                                                                                 \
            NDBG("setConfig needs 2 parameters, only " + String(numData) + " provided."); \
            return false;                                                                 \
        }                                                                                 \
        else                                                                              \
        {
#define ElifCommandCheck(cmd, Count) EndCommandCheck else CommandCheck(cmd, Count)
#define EndCommandCheck \
    }                   \
    }

// Parameters

// Class-less parameter system
#define DeclareBoolParam(name, val) bool name = val;
#define DeclareIntParam(name, val) int name = val;
#define DeclareEnumParam(name, val) \
    int name = val;                 \
    bool##name##isEnum = true;
#define DeclareFloatParam(name, val) float name = val;
#define DeclareStringParam(name, val) String name = val;
#define DeclareP2DParam(name, val1, val2) float name[2]{val1, val2};
#define DeclareP3DParam(name, val1, val2, val3) float name[3]{val1, val2, val3};
#define DeclareColorParam(name, r, g, b, a) float name[4]{r, b, g, a};

#define AddParamWithTag(type, class, param, tag) \
    addParam(&param, ParamType::type, tag);      \
    SetParam(param, Settings::getVal<class>(o, #param, param));

#define AddMultiParamWithTag(type, class, param, tag, numData) \
    {                                                          \
                                                               \
        addParam(&param, ParamType::type, tag);                \
        if (o.containsKey(#param))                             \
        {                                                      \
            JsonArray vArr = o[#param].as<JsonArray>();        \
            var dataV[numData];                                \
            dataV[0] = vArr[0].as<class>();                    \
            dataV[1] = vArr[1].as<class>();                    \
            if (numData > 2)                                   \
                dataV[2] = vArr[2].as<class>();                \
            setParam(param, dataV, numData);                   \
        }                                                      \
    }

#define AddBoolParamWithTag(param, tag) AddParamWithTag(Bool, bool, param, tag)
#define AddIntParamWithTag(param, tag) AddParamWithTag(Int, int, param, tag)
#define AddFloatParamWithTag(param, tag) AddParamWithTag(Float, float, param, tag)
#define AddStringParamWithTag(param, tag) AddParamWithTag(Str, String, param, tag)
#define AddP2DParamWithTag(param, tag) AddMultiParamWithTag(P2D, float, param, tag, 2);
#define AddP3DParamWithTag(param, tag) AddMultiParamWithTag(P3D, float, param, tag, 3);
#define AddColorParamWithTag(param, tag) AddMultiParamWithTag(TypeColor, float, param, tag, 4);

#define AddBoolParam(param) AddBoolParamWithTag(param, TagNone)
#define AddIntParam(param) AddIntParamWithTag(param, TagNone)
#define AddFloatParam(param) AddFloatParamWithTag(param, TagNone)
#define AddStringParam(param) AddStringParamWithTag(param, TagNone)
#define AddP2DParam(param) AddP2DParamWithTag(param, TagNone)
#define AddP3DParam(param) AddP3DParamWithTag(param, TagNone)
#define AddColorParam(param) AddColorParamWithTag(param, TagNone)

#define AddBoolParamConfig(param) AddBoolParamWithTag(param, TagConfig)
#define AddIntParamConfig(param) AddIntParamWithTag(param, TagConfig)
#define AddFloatParamConfig(param) AddFloatParamWithTag(param, TagConfig)
#define AddStringParamConfig(param) AddStringParamWithTag(param, TagConfig)
#define AddP2DParamConfig(param) AddP2DParamWithTag(param, TagConfig)
#define AddP3DParamConfig(param) AddP3DParamWithTag(param, TagConfig)
#define AddColorParamConfig(param) AddColorParamWithTag(param, TagConfig)

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
        setParam(param, pData, 2);   \
    };
#define SetParam3(param, val1, val2, val3) \
    {                                      \
        var pData[3];                      \
        pData[0] = val1;                   \
        pData[1] = val2;                   \
        pData[2] = val3;                   \
        setParam(param, pData, 3);         \
    };

// Handle Check and Set

#define HandleSetParamInternalStart                                                               \
    virtual bool handleSetParamInternal(const String &paramName, var *data, int numData) override \
    {

#define HandleSetParamInternalMotherClass(Class)                 \
    if (Class::handleSetParamInternal(paramName, data, numData)) \
        return true;

#define CheckTrigger(func)  \
    if (paramName == #func) \
    {                       \
        func();             \
        return true;        \
    }

#define CheckAndSetParam(param)                      \
    {                                                \
        if (paramName == #param)                     \
        {                                            \
            setParam((void *)&param, data, numData); \
            return true;                             \
        }                                            \
    }

#define CheckAndSetEnumParam(param, options, numOption)                   \
    {                                                                     \
        if (paramName == #param)                                          \
        {                                                                 \
            var newData[1];                                               \
            if (data[0].type == 's')                                      \
            {                                                             \
                String s = data[0].stringValue();                         \
                for (int i = 0; i < numOption; i++)                       \
                {                                                         \
                    if (s == options[i])                                  \
                    {                                                     \
                        newData[0] = i;                                   \
                        break;                                            \
                    }                                                     \
                };                                                        \
            }                                                             \
            else                                                          \
                newData[0] = data[0].intValue();                          \
            DBG(String("Set enum param to ") + newData[0].stringValue()); \
            setParam((void *)&param, newData, 1);                         \
            return true;                                                  \
        }                                                                 \
    }

#define HandleSetParamInternalEnd \
    return false;                 \
    }

// Feedback

#define CheckFeedbackParamInternalStart                   \
    virtual bool checkParamsFeedbackInternal(void *param) \
    {

#define CheckFeedbackParamInternalMotherClass(Class) \
    if (Class::checkParamsFeedbackInternal(param))   \
        return true;

#define CheckAndSendParamFeedback(p) \
    {                                \
        if (param == (void *)&p)     \
        {                            \
            SendParamFeedback(p);    \
            return true;             \
        }                            \
    }

#define CheckFeedbackParamInternalEnd \
    return false;                     \
    }

#define SendParamFeedback(param) CommunicationComponent::instance->sendParamFeedback(this, &param, #param, getParamType(&param));
#define SendMultiParamFeedback(param) CommunicationComponent::instance->sendParamFeedback(this, param, #param, getParamType(&param));

#define GetEnumStringStart                                   \
    virtual String getEnumString(void *param) const override \
    {

#define GetEnumStringEnd \
    return "";           \
    }

#define GetEnumStringParam(p, options, numOptions) \
    if (param == &p)                               \
        return options[p];

// Fill Settings

#define FillSettingsParam(param) \
    {                            \
        o[#param] = param;       \
    }

#define FillSettingsParam2(param)                     \
    {                                                 \
        JsonArray pArr = o.createNestedArray(#param); \
        pArr[0] = param[0];                           \
        pArr[1] = param[1];                           \
    }

#define FillSettingsParam3(param)                     \
    {                                                 \
        JsonArray pArr = o.createNestedArray(#param); \
        pArr[0] = param[0];                           \
        pArr[1] = param[1];                           \
        pArr[2] = param[2];                           \
    }

#define FillSettingsParam4(param)                     \
    {                                                 \
        JsonArray pArr = o.createNestedArray(#param); \
        pArr[0] = param[0];                           \
        pArr[1] = param[1];                           \
        pArr[2] = param[2];                           \
        pArr[3] = param[3];                           \
    }

#define FillSettingsInternalMotherClass(Class) Class::fillSettingsParamsInternal(o, showConfig);

#define FillSettingsInternalStart                                                           \
    virtual void fillSettingsParamsInternal(JsonObject o, bool showConfig = false) override \
    {
#define FillSettingsInternalEnd }

// Fill OSCQuery
#define FillOSCQueryTrigger(func) fillOSCQueryParam(o, fullPath, #func, ParamType::Trigger, nullptr, showConfig);
#define FillOSCQueryBoolParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Bool, &param, showConfig);
#define FillOSCQueryIntParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Int, &param, showConfig);
#define FillOSCQueryEnumParam(param, options, numOptions) fillOSCQueryParam(o, fullPath, #param, ParamType::Int, &param, showConfig, false, options, numOptions);
#define FillOSCQueryFloatParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Float, &param, showConfig);
#define FillOSCQueryRangeParam(param, vMin, vMax) fillOSCQueryParam(o, fullPath, #param, ParamType::Float, &param, showConfig, false, nullptr, 0, vMin, vMax);
#define FillOSCQueryStringParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Str, &param, showConfig);
#define FillOSCQueryP2DParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::P2D, param, showConfig);
#define FillOSCQueryP3DParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::P2D, param, showConfig);
#define FillOSCQueryColorParam(param) fillOSCQueryParam(o, fullPath, #param, ParamType::TypeColor, param, showConfig);

#define FillOSCQueryTriggerReadOnly(func) fillOSCQueryParam(o, fullPath, #func, ParamType::Trigger, nullptr, showConfig, true);
#define FillOSCQueryBoolParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Bool, &param, showConfig, true);
#define FillOSCQueryIntParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Int, &param, showConfig, true);
#define FillOSCQueryEnumParamReadOnly(param, options, numOptions) fillOSCQueryParam(o, fullPath, #param, ParamType::Int, &param, showConfig, true, options, numOptions);
#define FillOSCQueryFloatParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Float, &param, showConfig, true);
#define FillOSCQueryRangeParamReadOnly(param, vMin, vMax) fillOSCQueryParam(o, fullPath, #param, ParamType::Float, &param, showConfig, true, nullptr, 0, vMin, vMax);
#define FillOSCQueryStringParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::Str, &param, showConfig, true);
#define FillOSCQueryP2DParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::P2D, param, showConfig, true);
#define FillOSCQueryP2DRangeParamReadOnly(param, min1, max1, min2, max2) fillOSCQueryParam(o, fullPath, #param, ParamType::P3D, param, showConfig, true, nullptr, 0, min1, max1, min2, max2);
#define FillOSCQueryP3DParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::P3D, param, showConfig, true);
#define FillOSCQueryP3DRangeParamReadOnly(param, min1, max1, min2, max2, min3, max3) fillOSCQueryParam(o, fullPath, #param, ParamType::P3D, param, showConfig, true, nullptr, 0, min1, max1, min2, max2, min3, max3);
#define FillOSCQueryColorParamReadOnly(param) fillOSCQueryParam(o, fullPath, #param, ParamType::TypeColor, param, showConfig, true);

#define FillOSCQueryInternalStart                                                                         \
    virtual void fillOSCQueryParamsInternal(JsonObject o, const String &fullPath, bool showConfig = true) \
    {
#define FillOSCQueryInternalEnd }

#define FillOSCQueryInternalMotherClass(Class) Class::fillOSCQueryParamsInternal(o, fullPath, showConfig);

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