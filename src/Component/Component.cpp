#include "UnityIncludes.h"

void Component::setup(JsonObject o)
{
    NDBG("Setup");
    AddBoolParamConfig(enabled);
    setupInternal(o);
}

bool Component::init()
{
    if (!enabled)
        return false;

    NDBG("Init");

    for (int i = 0; i < numComponents; i++)
    {
        // NDBG("> Init " + components[i]->name);
        if (!components[i]->enabled)
            continue;
        components[i]->init();
    }

    isInit = initInternal();

    if (!isInit)
        NDBG("Init Error.");
    // else
    //     NDBG(F("Init OK"));

    return isInit;
}

void Component::update()
{
    // NDBG("Looping");
    if (!enabled)
        return;

    long currentTime = millis();
    if (lastUpdateTime > 0 && updateRate > 0 && currentTime - lastUpdateTime < (1000 / updateRate))
        return;

    lastUpdateTime = currentTime;

    for (int i = 0; i < numComponents; i++)
        components[i]->update();

    updateInternal();
}

void Component::clear()
{
    NDBG("Clear " + name);

    clearInternal();

    for (int i = 0; i < numComponents; i++)
    {
        components[i]->clear();
        // delete components[i];
    }

    numComponents = 0;

    // for (int i = 0; i < numParameters; i++)
    // delete parameters[i];
    numParams = 0;
}

bool Component::handleCommand(const String &command, var *data, int numData)
{
    if (numData == 0)
    {
        if (triggersMap.contains(command))
        {
            triggersMap.at(command)();
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

bool Component::checkCommand(const String &command, const String &ref, int numData, int expectedData)
{
    if (command != ref)
        return false;
    if (numData < expectedData)
    {
        NDBG("Command " + command + " expects at least " + expectedData + " arguments");
        return false;
    }
    return true;
}

// Save / Load

void Component::fillSettingsData(JsonObject o)
{
    for (int i = 0; i < numParams; i++)
    {
        void *param = params[i];

        if (param == &enabled && !saveEnabled)
            continue;

        bool isReadOnly = checkParamTag(param, TagFeedback);
        if (isReadOnly)
            continue;
        fillSettingsParam(o, paramToNameMap.at(param), param);
    }

    if (numComponents > 0)
    {
        JsonObject comps = o.createNestedObject("components");
        for (int i = 0; i < numComponents; i++)
        {
            Component *c = components[i];
            JsonObject co = comps.createNestedObject(c->name);
            c->fillSettingsData(co);
        }
    }
}

void Component::fillSettingsParam(JsonObject o, const String &pName, void *param)
{
    ParamType t = getParamType(param);

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
        o[pName] = (*(String *)param);
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
        o[pName] = getEnumString(param);
        break;

    default:
        break;
    }
}

void Component::fillChunkedOSCQueryData(OSCQueryChunk *chunk, bool showConfig)
{
    const String fullPath = getFullPath(); // this == RootComponent::instance);

    switch (chunk->nextType)
    {
    case Start:
    {
        chunk->data = "{\"DESCRIPTION\":\"" + StringHelpers::lowerCamelToTitleCase(name) + "\"," +
                      "\"FULL_PATH\":\"" + fullPath + "\"," +
                      "\"ACCESS\":0," +
                      "\"CONTENTS\":";

        if (numParams > 0)
        {

            StaticJsonDocument<6000> doc;
            JsonObject o = doc.to<JsonObject>();

            for (int i = 0; i < numParams; i++)
            {
                void *param = params[i];

                if (param == &enabled && !exposeEnabled)
                    continue;

                fillOSCQueryParam(o, fullPath, param, showConfig);
            }

            String str;
            serializeJson(o, str);
            str.remove(str.length() - 1);
            chunk->data += str;
        }
        else
        {
            chunk->data += "{";
        }

        if (numComponents > 0)
        {
            chunk->nextType = Start;
            chunk->nextComponent = components[0];
            if (numParams > 0)
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

void Component::fillOSCQueryParam(JsonObject o, const String &fullPath, void *param, bool showConfig)
{
    bool isConfig = checkParamTag(param, TagConfig);

    if (!showConfig && isConfig)
        return;

    const String pName = paramToNameMap.at(param);
    ParamType t = getParamType(param);
    bool readOnly = checkParamTag(param, TagFeedback);

    JsonObject po = o.createNestedObject(pName);
    po["DESCRIPTION"] = StringHelpers::lowerCamelToTitleCase(pName);
    po["ACCESS"] = readOnly ? 1 : 3;
    const String pType = t == Bool ? (*(bool *)param) ? "T" : "F" : typeNames[t];
    po["TYPE"] = pType;
    po["FULL_PATH"] = fullPath + "/" + pName;

    if (isConfig)
    {
        JsonArray to = po.createNestedArray("TAGS");
        to.add("config");
    }

    if (t != ParamType::Trigger)
    {
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
            int numOptions = enumOptionsCountMap.contains(param) ? enumOptionsCountMap.at(param) : 0;
            const String *options = enumOptionsMap.contains(param) ? enumOptionsMap.at(param) : nullptr;
            if (options != nullptr && numOptions > 0)
            {
                JsonArray rArr = po.createNestedArray("RANGE");
                JsonObject vals = rArr.createNestedObject();
                JsonArray opt = vals.createNestedArray("VALS");

                for (int i = 0; i < numOptions; i++)
                {
                    opt.add(options[i]);
                }

                po["TYPE"] = "s"; // force string type
                int index = *(int *)param;
                if (index >= 0 && index < numOptions)
                    vArr.add(options[index]);
            }
        }
        break;

        case ParamType::Float:
            vArr.add((*(float *)param));
            break;

        case ParamType::Str:
            vArr.add((*(String *)param));
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
    }

    if (paramRangesMap.contains(param))
    {
        ParamRange range = paramRangesMap.at(param);
        float vMin1 = range.vMin;
        float vMax1 = range.vMax;
        float vMin2 = range.vMin2;
        float vMax2 = range.vMax2;
        float vMin3 = range.vMin3;
        float vMax3 = range.vMax3;
        if (vMin1 != vMax1)
        {
            JsonArray rArr = po.createNestedArray("RANGE");
            JsonObject r1 = rArr.createNestedObject();
            r1["MIN"] = vMin1;
            r1["MAX"] = vMax1;
            if (t == ParamType::P2D || t == ParamType::P3D)
            {
                JsonObject r2 = rArr.createNestedObject();

                r2["MIN"] = vMin2;
                r2["MAX"] = vMax2;

                if (t == ParamType::P3D)
                {
                    JsonObject r3 = rArr.createNestedObject();
                    r3["MIN"] = vMin3;
                    r3["MAX"] = vMax3;
                }
            }
        }
    }
}

void Component::setupChunkAfterComponent(OSCQueryChunk *chunk, const Component *c)
{
    int index = 0;
    for (int i = 0; i < numComponents; i++)
    {
        if (components[i] == c)
        {
            index = i;
            break;
        }
    }

    if (index < numComponents - 1) // last one
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

String Component::getFullPath(bool includeRoot, bool scriptMode, bool serialMode) const
{
    if (this == RootComponent::instance && !includeRoot)
        return "";

    Component *pc = parentComponent;
    String s = name;

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

    for (int i = 0; i < numComponents; i++)
    {
        if (components[i] == nullptr)
            continue;
        components[i]->linkScriptFunctions(module);
    }
}
#endif

Component *Component::addComponent(Component *c, JsonObject o)
{
    if (numComponents >= MAX_CHILD_COMPONENTS)
    {
        NDBG("Component limit reached ! Trying to add " + c->name);
        return nullptr;
    }

    components[numComponents] = (Component *)c;
    c->parentComponent = this;
    AddDefaultComponentListener(c);
    numComponents++;

    c->setup(o);
    // DBG("Component added, size = " + String(sizeof(*c)) + " (" + String(sizeof(Component)) + ")");
    return c;
}

Component *Component::getComponentWithName(const String &name)
{
    if (name == this->name)
        return this;

    int subCompIndex = name.indexOf('.');

    if (subCompIndex > 0)
    {
        String n = name.substring(0, subCompIndex);
        for (int i = 0; i < numComponents; i++)
        {
            if (components[i]->name == n)
                return components[i]->getComponentWithName(name.substring(subCompIndex + 1));
        }
    }
    else
    {
        for (int i = 0; i < numComponents; i++)
        {
            if (components[i]->name == name)
                return components[i];
        }
    }

    return NULL;
}

void Component::addParam(void *param, ParamType type, const String &paramName, uint8_t tags)
{
    if (numParams >= MAX_CHILD_PARAMS)
    {
        NDBG("Param limit reached !");
        return;
    }

    params[numParams] = param;
    paramTypesMap.insert(std::make_pair(param, type));
    paramTagsMap.insert(std::make_pair(param, tags));
    nameToParamMap.insert(std::make_pair(paramName, param));
    paramToNameMap.insert(std::make_pair(param, paramName));

    numParams++;
}

void Component::setParamRange(void *param, ParamRange range)
{
    paramRangesMap.emplace(param, range);
}

void Component::setEnumOptions(void *param, const String *enumOptions, int numOptions)
{
    enumOptionsMap.emplace(param, enumOptions);
    enumOptionsCountMap.emplace(param, numOptions);
}

void Component::setParam(void *param, var *value, int numData)
{
    bool hasChanged = false;
    ParamType t = getParamType(param);

    if (numData < 2 && t == P2D)
    {
        NDBG("Expecting at least 2 parameters");
        return;
    }

    if (numData < 3 && t == P3D)
    {
        NDBG("Expecting at least 3 parameters");
        return;
    }

    // if (numData < 4 && t == TypeColor)
    // {
    //     NDBG("Expecting at least 4 parameters");
    //     return;
    // }

    switch (t)
    {
    case ParamType::Trigger:

        break;

    case ParamType::Bool:

        hasChanged = *((bool *)param) != value[0].boolValue();

        // DBG("Set bool " + String(value[0].boolValue()) + " / " + String(*((bool *)param)) +" has Changed ? "+String(hasChanged));
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
        hasChanged = *((String *)param) != value[0].stringValue();
        if (hasChanged)
            *((String *)param) = value[0].stringValue();
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
        paramValueChanged(param);
    }
}

bool Component::handleSetParam(const String &paramName, var *data, int numData)
{
    if (nameToParamMap.contains(paramName))
    {
        setParam(nameToParamMap.at(paramName), data, numData);
        return true;
    }

    return false;
}

void Component::paramValueChanged(void *param)
{
    // DBG("Param value changed " + getParamString(param));

    if (param == &enabled)
        onEnabledChanged();

    paramValueChangedInternal(param);

    checkParamsFeedback(param);

    if (parentComponent != nullptr)
        parentComponent->childParamValueChanged(this, this, param);
}

void Component::childParamValueChanged(Component *caller, Component *comp, void *param)
{
    // NDBG("Child param value changed : "+caller->name + " > " + comp->name);
    if (parentComponent != nullptr)
        parentComponent->childParamValueChanged(this, comp, param);
}

bool Component::checkParamsFeedback(void *param)
{
    if (feedbackRate < 0)
        return false;

    if (!checkParamTag(param, TagFeedback))
        return false;

    bool shouldSend = false;
    long currentTime = millis();
    if (lastFeedbackTime > 0 && feedbackRate > 0 && (currentTime - lastFeedbackTime) < (1000.0f / feedbackRate))
    {
        return false;
    }

    CommunicationComponent::instance->sendParamFeedback(this, param, paramToNameMap.at(param), getParamType(param));
    return true;
}

Component::ParamType Component::getParamType(void *param) const
{
    paramTypesMap.at(param);
    return ParamType::ParamTypeMax;
}

bool Component::checkParamTag(void *param, ParamTag tag) const
{
    return (paramTagsMap.at(param) & tag) != 0;
}

String Component::getParamString(void *param) const
{
    ParamType t = getParamType(param);
    switch (t)
    {
    case ParamType::Bool:
        return String(*((bool *)param));

    case ParamType::Int:
        return String(*((int *)param));

    case ParamType::TypeEnum:
        return getEnumString(param);

    case ParamType::Float:
        return String(*((float *)param));

    case ParamType::Str:
        return *((String *)param);

    case ParamType::P2D:
        return String(((float *)param)[0]) + ", " + String(((float *)param)[1]);

    case ParamType::P3D:
        return String(((float *)param)[0]) + ", " + String(((float *)param)[1]) + ", " + String(((float *)param)[2]);

    case ParamType::TypeColor:
        return String(((float *)param)[0]) + ", " + String(((float *)param)[1]) + ", " + String(((float *)param)[2]) + ", " + String(((float *)param)[3]);

    default:
        break;
    }

    return String("[unknown]");
}

void Component::addTrigger(const String &name, std::function<void(void)> func)
{
    triggersMap.emplace(name, func);
}
