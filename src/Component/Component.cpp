#include "UnityIncludes.h"
#include "Component.h"

void Component::setup(JsonObject o)
{
    NDBG("Setup");
    AddBoolParamConfig(enabled);
    setupInternal(o);
    RootComponent::instance->registerComponent(this, getFullPath(false, true), isHighPriority);
}

bool Component::init()
{
    if (!enabled)
        return false;

    NDBG("Init");

    for (auto &c : components)
    {
        if (!c->enabled)
            continue;
        c->init();
    }

    isInit = initInternal();

    if (!isInit)
        NDBG("Init Error.");

    return isInit;
}

void Component::update(bool inFastLoop)
{
    if (!enabled)
        return;

    long currentTime = millis();
    if (updateRate > 0)
    {
        if (currentTime - lastUpdateTime < (1000 / updateRate))
            return;
    }

    lastUpdateTime = currentTime;

    if (isHighPriority == inFastLoop)
    {
        for (auto &c : components)
        {
            c->update(inFastLoop);
        }

        long startTime = millis();
        updateInternal();
        long endTime = millis();
        long duration = endTime - startTime;
        if (duration > 5)
        {
            NDBG("Component " + name + " update took " + std::to_string(duration) + " ms");
        }
    }
}

void Component::clear()
{
    NDBG("Clear " + name);

    clearInternal();

    for (auto &c : components)
    {
        c->clear();
    }
    components.clear();

    // for (int i = 0; i < numParameters; i++)
    // delete parameters[i];
    params.clear();
    RootComponent::instance->unregisterComponent(this);
}

void Component::setCustomUpdateRate(int defaultRate, JsonObject o)
{
    if (updateRate < 0)
        return;
    updateRate = defaultRate;
    AddIntParamConfig(updateRate);
}

void Component::setCustomFeedbackRate(float defaultRate, JsonObject o)
{
    if (feedbackRate < 0)
        return;
    feedbackRate = defaultRate;
    AddFloatParamConfig(feedbackRate);
}

void Component::sendEvent(uint8_t type, var *data, int numData)
{
    EventBroadcaster::sendEvent(ComponentEvent(this, type, data, numData));
}

bool Component::handleCommand(const std::string &command, var *data, int numData)
{
    if (numData == 0)
    {
        Trigger *trigger = getTrigger(command);
        if (trigger)
        {
            trigger->func();
            return true;
        }
    }
    else
    {
        if (handleSetParam(command, data, numData))
            return true;
    }

    if (handleCommandInternal(command, data, numData))
        return true;

    return false;
}

bool Component::checkCommand(const std::string &command, const std::string &ref, int numData, int expectedData)
{
    if (command != ref)
        return false;
    if (numData < expectedData)
    {
        NDBG("Command " + command + " expects at least " + std::to_string(expectedData) + " arguments");
        return false;
    }
    return true;
}

// Save / Load

void Component::fillSettingsData(JsonObject o)
{
    for (auto &param : params)
    {
        if (param.ptr == &enabled && !saveEnabled)
            continue;

        if (param.hasTag(TagFeedback))
            continue;

        fillSettingsParam(o, param);
    }

    if (components.size() > 0)
    {
        JsonObject comps = o.createNestedObject("components");
        for (auto &c : components)
        {
            JsonObject co = comps.createNestedObject(c->name);
            c->fillSettingsData(co);
        }
    }
}

void Component::fillSettingsParam(JsonObject o, const ParamInfo &paramInfo)
{
    ParamType t = paramInfo.type;
    const std::string pName = paramInfo.name;
    void *param = paramInfo.ptr;

    switch (t)
    {
    case ParamType::Bool:
        o[pName] = (*(bool *)param);
        break;

    case ParamType::Int:
        o[pName] = (*(int *)param);
        break;

    case ParamType::Float:
        o[pName] = (*(float *)param);
        break;

    case ParamType::Str:
        o[pName] = (*(std::string *)param);
        break;

    case ParamType::P2D:
        o[pName] = JsonArray();
        o[pName][0] = ((float *)(param))[0];
        o[pName][1] = ((float *)(param))[1];
        break;

    case ParamType::P3D:
        o[pName] = JsonArray();
        o[pName][0] = ((float *)(param))[0];
        o[pName][1] = ((float *)(param))[1];
        o[pName][2] = ((float *)(param))[2];
        break;

    case ParamType::TypeColor:
        o[pName] = (*(uint32_t *)param);
        break;

    case ParamType::TypeEnum:
        o[pName] = (*(int *)param);
        break;

    default:
        DBG("Unsupported param type for saving settings: " + paramInfo.getTypeString());
        break;
    }
}

void Component::fillChunkedOSCQueryData(OSCQueryChunk *chunk, bool showConfig)
{
    const std::string fullPath = getFullPath(); // this == RootComponent::instance);

    switch (chunk->nextType)
    {
    case Start:
    {
        chunk->data = "{\"DESCRIPTION\":\"" + StringHelpers::lowerCamelToTitleCase(name) + "\"," +
                      "\"FULL_PATH\":\"" + fullPath + "\"," +
                      "\"ACCESS\":0," +
                      "\"CONTENTS\":";

        int numParamsSaved = 0;

        // iterates through trigger map

        if (params.size() > 0 || triggers.size() > 0)
        {
            StaticJsonDocument<6000> doc;
            JsonObject o = doc.to<JsonObject>();

            if (triggers.size() > 0)
            {
                for (const auto &t : triggers)
                {
                    createBaseOSCQueryObject(o, fullPath, t.name, "I", false);
                    numParamsSaved++;
                }
            }

            for (auto &param : params)
            {
                if (param.ptr == &enabled && !exposeEnabled)
                    continue;

                if (fillOSCQueryParam(o, fullPath, param, showConfig))
                {
                    numParamsSaved++;
                }
            }

            std::string str;
            serializeJson(o, str);
            str.erase(str.length() - 1);
            chunk->data += str;
        }
        else
        {
            chunk->data += "{";
        }

        if (components.size() > 0)
        {
            chunk->nextType = Start;
            chunk->nextComponent = components[0];
            if (numParamsSaved > 0)
                chunk->data += ",";
            chunk->data += "\"" + components[0]->name + "\":";
        }
        else
        {
            chunk->data += "}";
            chunk->nextComponent = (Component *)this;
            chunk->nextType = End;
        }
    }
    break;

    case End:
    {
        chunk->data = "}";
        if (parentComponent != nullptr)
        {
            parentComponent->setupChunkAfterComponent(chunk, this);
        }
        else
            chunk->nextComponent = nullptr;
    }

    default:
        break;
    }
}

bool Component::fillOSCQueryParam(JsonObject o, const std::string &fullPath, const ParamInfo &paramInfo, bool showConfig)
{
    if (!showConfig && paramInfo.hasTag(TagConfig))
        return false;

    const std::string pName = paramInfo.name;
    ParamType t = paramInfo.type;
    bool readOnly = paramInfo.hasTag(TagFeedback);
    std::string paramTypeStr = paramInfo.getTypeString();
    void *param = paramInfo.ptr;
    JsonObject po = createBaseOSCQueryObject(o, fullPath, pName, t == Bool ? (*(bool *)param) ? "T" : "F" : paramTypeStr, readOnly);
    JsonArray vArr = po.createNestedArray("VALUE");

    switch (t)
    {
    case ParamType::Bool:
        vArr.add((*(bool *)param));
        break;

    case ParamType::Int:
        vArr.add((*(int *)param));
        break;

    case ParamType::TypeEnum:
    {
        const std::string *options = paramInfo.enumOptions;
        if (options != nullptr && paramInfo.numEnumOptions > 0)
        {
            JsonArray rArr = po.createNestedArray("RANGE");
            JsonObject vals = rArr.createNestedObject();
            JsonArray opt = vals.createNestedArray("VALS");

            for (int i = 0; i < paramInfo.numEnumOptions; i++)
            {
                opt.add(options[i]);
            }

            po["TYPE"] = "s"; // force string type
            int index = *(int *)param;
            if (index >= 0 && index < paramInfo.numEnumOptions)
                vArr.add(options[index]);
        }
    }
    break;

    case ParamType::Float:
        vArr.add((*(float *)param));
        break;

    case ParamType::Str:
        vArr.add((*(std::string *)param));
        break;

    case ParamType::P2D:
        vArr.add(((float *)param)[0]);
        vArr.add(((float *)param)[1]);
        break;

    case ParamType::P3D:
        vArr.add(((float *)param)[0]);
        vArr.add(((float *)param)[1]);
        vArr.add(((float *)param)[2]);
        break;

    case ParamType::TypeColor:
        vArr.add(((float *)param)[0]);
        vArr.add(((float *)param)[1]);
        vArr.add(((float *)param)[2]);
        vArr.add(((float *)param)[3]);
        break;

    default:
        break;
    }

    if (param)
    {
        ParamRange *range = paramInfo.range;
        if (range != nullptr && range->vMin != range->vMax)
        {
            JsonArray rArr = po.createNestedArray("RANGE");
            JsonObject r1 = rArr.createNestedObject();
            r1["MIN"] = range->vMin;
            r1["MAX"] = range->vMax;
            if (t == ParamType::P2D || t == ParamType::P3D)
            {
                JsonObject r2 = rArr.createNestedObject();

                r2["MIN"] = range->vMin2;
                r2["MAX"] = range->vMax2;

                if (t == ParamType::P3D)
                {
                    JsonObject r3 = rArr.createNestedObject();
                    r3["MIN"] = range->vMin3;
                    r3["MAX"] = range->vMax3;
                }
            }
        }
    }

    return true;
}

JsonObject Component::createBaseOSCQueryObject(JsonObject o, const std::string &fullPath, const std::string &pName, const std::string &type, bool readOnly)
{
    JsonObject obj = o.createNestedObject(pName);
    obj["DESCRIPTION"] = StringHelpers::lowerCamelToTitleCase(pName);
    obj["ACCESS"] = readOnly ? 1 : 3;
    obj["TYPE"] = type;
    obj["FULL_PATH"] = fullPath + "/" + pName;
    return obj;
}

void Component::setupChunkAfterComponent(OSCQueryChunk *chunk, const Component *c)
{

    int index = -1;
    for (size_t i = 0; i < components.size(); i++)
    {
        if (components[i] == c)
        {
            index = i;
            break;
        }
    }

    if (index < components.size() - 1) // last one
    {
        chunk->data += ",\"" + components[index + 1]->name + "\":";
        chunk->nextComponent = components[index + 1];
        chunk->nextType = Start;
    }
    else
    {
        chunk->data += "}";
        chunk->nextComponent = this;
        chunk->nextType = End;
    }
}

std::string Component::getFullPath(bool includeRoot, bool scriptMode, bool serialMode) const
{
    if (this == RootComponent::instance && !includeRoot)
        return "";

    Component *pc = parentComponent;
    std::string s = name;

    char separator = '/';
    if (scriptMode)
        separator = '.';
    else if (serialMode)
        separator = '_';

    while (pc != NULL)
    {
        if (pc == RootComponent::instance && !includeRoot)
            break;
        s = pc->name + separator + s;
        pc = pc->parentComponent;
    }

    if (!scriptMode)
        s = "/" + s;

    return s;
}

// Scripts
#ifdef USE_SCRIPT
void Component::linkScriptFunctions(IM3Module module, bool isLocal)
{
    const char *tName = isLocal ? "local" : name.c_str(); // getFullPath(false, true).c_str();

    // LinkScriptFunction(Component, setEnabled, v, i);

    linkScriptFunctionsInternal(module, tName);

    for (auto &c : components)
    {
        if (c == nullptr)
            continue;
        c->linkScriptFunctions(module);
    }
}
#endif

Component *Component::addComponent(Component *c, JsonObject o)
{
    components.push_back(c);
    c->parentComponent = this;
    AddDefaultComponentListener(c);

    c->setup(o);
    // DBG("Component added, size = " + std::to_string(sizeof(*c)) + " (" + std::to_string(sizeof(Component)) + ")");
    return c;
}

ParamInfo *Component::addParam(void *param, ParamType type, const std::string &paramName, uint8_t tags)
{
    params.push_back({param, paramName, type, tags});
    return &params.back();
}

void Component::setParam(void *param, var *value, int numData)
{
    bool hasChanged = false;
    ParamInfo *pInfo = getParamInfo(param);
    if (pInfo == nullptr)
    {
        NDBG("Parameter not found");
        return;
    }

    if (numData < 2 && pInfo->type == P2D)
    {
        NDBG("Expecting at least 2 parameters");
        return;
    }

    if (numData < 3 && pInfo->type == P3D)
    {
        NDBG("Expecting at least 3 parameters");
        return;
    }

    // if (numData < 4 && t == TypeColor)
    // {
    //     NDBG("Expecting at least 4 parameters");
    //     return;
    // }

    switch (pInfo->type)
    {
    case ParamType::Bool:

        hasChanged = *((bool *)param) != value[0].boolValue();

        // DBG("Set bool " + std::to_string(value[0].boolValue()) + " / " + std::to_string(*((bool *)param)) +" has Changed ? "+String(hasChanged));
        if (hasChanged)
            *((bool *)param) = value[0].boolValue();
        break;

    case ParamType::Int:
        hasChanged = *((int *)param) != value[0].intValue();
        if (hasChanged)
            *((int *)param) = value[0].intValue();
        break;

    case ParamType::TypeEnum:
        hasChanged = *((int *)param) != value[0].intValue();
        if (hasChanged)
            *((int *)param) = value[0].intValue();
        break;

    case ParamType::Float:
        hasChanged = *((float *)param) != value[0].floatValue();
        if (hasChanged)
            *((float *)param) = value[0].floatValue();
        break;

    case ParamType::Str:
        hasChanged = *((std::string *)param) != value[0].stringValue();
        if (hasChanged)
            *((std::string *)param) = value[0].stringValue();
        break;

    case ParamType::P2D:
        hasChanged = ((float *)param)[0] != value[0].floatValue() || ((float *)param)[1] != value[1].floatValue();
        if (hasChanged)
        {
            ((float *)param)[0] = value[0].floatValue();
            ((float *)param)[1] = value[1].floatValue();
        }
        break;

    case ParamType::P3D:
        hasChanged = ((float *)param)[0] != value[0].floatValue() || ((float *)param)[1] != value[1].floatValue() || ((float *)param)[2] != value[2].floatValue();
        if (hasChanged)
        {
            ((float *)param)[0] = value[0].floatValue();
            ((float *)param)[1] = value[1].floatValue();
            ((float *)param)[2] = value[2].floatValue();
        }
        else
        {
            // DBG("No change");
        }
        break;

    case ParamType::TypeColor:
        if (numData >= 4)
        {
            hasChanged = ((float *)param)[0] != value[0].floatValue() || ((float *)param)[1] != value[1].floatValue() || ((float *)param)[2] != value[2].floatValue() || ((float *)param)[3] != value[3].floatValue();
            if (hasChanged)
            {
                ((float *)param)[0] = value[0].floatValue();
                ((float *)param)[1] = value[1].floatValue();
                ((float *)param)[2] = value[2].floatValue();
                ((float *)param)[3] = value[3].floatValue();
            }
        }
        else if (numData == 3)
        {
            hasChanged = ((float *)param)[0] != value[0].floatValue() || ((float *)param)[1] != value[1].floatValue() || ((float *)param)[2] != value[2].floatValue();
            if (hasChanged)
            {
                ((float *)param)[0] = value[0].floatValue();
                ((float *)param)[1] = value[1].floatValue();
                ((float *)param)[2] = value[2].floatValue();
                ((float *)param)[3] = 1.0;
            }
        }
        else if (numData == 1)
        {
            Color c;

            if (value[0].type == 's')
            {
                c = Color(value[0].stringValue());
            }
            else
            {
                c = Color(value[0].intValue());
            }

            float r = c.getFloatRed();
            float g = c.getFloatGreen();
            float b = c.getFloatBlue();
            float a = c.getFloatAlpha();

            hasChanged = ((float *)param)[0] != r || ((float *)param)[1] != g || ((float *)param)[2] != b || ((float *)param)[3] != a;
            if (hasChanged)
            {
                ((float *)param)[0] = r;
                ((float *)param)[1] = g;
                ((float *)param)[2] = b;
                ((float *)param)[3] = a;
            }
        }
        break;

    default:
        // not handle
        break;
    }

    if (hasChanged)
    {
        // notify here
        paramValueChanged(pInfo);
    }
}

bool Component::handleSetParam(const std::string &paramName, var *data, int numData)
{
    if (ParamInfo *pInfo = getParamInfo(paramName))
    {
        setParam(pInfo->ptr, data, numData);
        return true;
    }

    return false;
}

void Component::paramValueChanged(ParamInfo *paramInfo)
{
    if (paramInfo->ptr == &enabled)
        onEnabledChanged();

    paramValueChangedInternal(paramInfo);

    checkParamsFeedback(paramInfo);

    if (parentComponent != nullptr)
        parentComponent->childParamValueChanged(this, this, paramInfo);
}

void Component::childParamValueChanged(Component *caller, Component *comp, ParamInfo*paramInfo)
{
    // NDBG("Child param value changed : "+caller->name + " > " + comp->name);
    if (parentComponent != nullptr)
        parentComponent->childParamValueChanged(this, comp, paramInfo);
}

bool Component::checkParamsFeedback(ParamInfo *paramInfo)
{
    if (feedbackRate < 0)
        return false;

    if (!paramInfo->hasTag(TagFeedback))
        return false;

    bool shouldSend = false;
    long currentTime = millis();
    if (lastFeedbackTime > 0 && feedbackRate > 0 && (currentTime - lastFeedbackTime) < (1000.0f / feedbackRate))
    {
        return false;
    }

    CommunicationComponent::instance->sendParamFeedback(this, paramInfo);
    return true;
}

void Component::addTrigger(const std::string &name, std::function<void(void)> func)
{
    triggers.push_back({name, func});
}

Trigger *Component::getTrigger(const std::string &name)
{
    for (auto &t : triggers)
    {
        if (t.name == name)
            return &t;
    }

    DBG("Trigger not found: " + name);
    return nullptr;
}

ParamInfo *Component::getParamInfo(void *param)
{
    for (auto &p : params)
    {
        if (p.ptr == param)
            return &p;
    }

    DBG("Param not found");
    return nullptr; // should not happen
}

ParamInfo *Component::getParamInfo(const std::string &paramName)
{
    for (auto &p : params)
    {
        if (p.name == paramName)
            return &p;
    }

    DBG("Param not found: " + paramName);
    return nullptr; // should not happen
}

void *Component::getParamByName(const std::string &paramName)
{
    ParamInfo *param = getParamInfo(paramName);
    if (param != nullptr)
        return param->ptr;
    return nullptr;
}
