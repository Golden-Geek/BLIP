#include "UnityIncludes.h"

void Component::setup(JsonObject o)
{
    NDBG("Setup");
    AddBoolParam(enabled);
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
    if (handleSetParam(command, data, numData))
        return true;

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

void Component::fillSettingsData(JsonObject o, bool showConfig)
{
    if (saveEnabled)
        FillSettingsParam(enabled);
    fillSettingsParamsInternal(o, showConfig);

    if (numComponents > 0)
    {
        JsonObject comps = o.createNestedObject("components");
        for (int i = 0; i < numComponents; i++)
        {
            Component *c = components[i];
            JsonObject co = comps.createNestedObject(c->name);
            c->fillSettingsData(co, showConfig);
        }
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

            if (exposeEnabled)
                FillOSCQueryBoolParam(enabled);
            fillOSCQueryParamsInternal(o, fullPath, showConfig);

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

void Component::fillOSCQueryParam(JsonObject o, const String &fullPath, const String &pName, ParamType t, void *param,
                                  bool showConfig, bool readOnly, const String *options, int numOptions,
                                  float vMin1, float vMax1, float vMin2, float vMax2, float vMin3, float vMax3)
{
    ParamTag tag = getParamTag(param);

    if (!showConfig && tag == TagConfig)
        return;

    JsonObject po = o.createNestedObject(pName);
    po["DESCRIPTION"] = StringHelpers::lowerCamelToTitleCase(pName);
    po["ACCESS"] = readOnly ? 1 : 3;
    const String pType = t == Bool ? (*(bool *)param) ? "T" : "F" : typeNames[t];
    po["TYPE"] = pType;
    po["FULL_PATH"] = fullPath + "/" + pName;

    if (tag != TagNone)
    {
        JsonArray to = po.createNestedArray("TAGS");
        to.add(tagNames[tag]);
    }

    if (t != ParamType::Trigger)
    {
        JsonArray vArr = po.createNestedArray("VALUE");

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
        else
        {
            switch (t)
            {
            case ParamType::Bool:
                vArr.add((*(bool *)param));
                break;

            case ParamType::Int:
                vArr.add((*(int *)param));
                break;

            case ParamType::TypeEnum:
                vArr.add(getEnumString(param));
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

        if (vMin1 != 0 || vMax1 != 0)
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

// Parameter *Component::getParameterWithName(const String &name)
// {
//     for (int i = 0; i < numParameters; i++)
//         if (parameters[i]->name == name)
//             return parameters[i];

//     return NULL;
// }

void Component::addParam(void *param, ParamType type, ParamTag tag)
{
    if (numParams >= MAX_CHILD_PARAMS)
    {
        NDBG("Param limit reached !");
        return;
    }

    params[numParams] = param;
    paramTypes[numParams] = type;
    paramTags[numParams] = tag;
    numParams++;
}

void Component::setParam(void *param, var *value, int numData)
{
    bool hasChanged = false;
    ParamType t = getParamType(param);

    // NDBG("Set Param " + String(t));

    if (numData == 0 && t != Trigger)
    {
        // Send current value feedback
        SendParamFeedback(param);
        // NDBG("Expecting at least 1 parameter");
        return;
    }

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
            // convert rgba uint32_t to 4 floats
            uint32_t rgba = value[0].intValue();
            float r = ((rgba >> 24) & 0xFF) / 255.0f;
            float g = ((rgba >> 16) & 0xFF) / 255.0f;
            float b = ((rgba >> 8) & 0xFF) / 255.0f;
            float a = (rgba & 0xFF) / 255.0f;

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
    CheckAndSetParam(enabled);

    return handleSetParamInternal(paramName, data, numData);
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
    CheckAndSendParamFeedback(enabled);
    return checkParamsFeedbackInternal(param);
}

Component::ParamType Component::getParamType(void *param) const
{
    for (int i = 0; i < numParams; i++)
        if (params[i] == param)
            return paramTypes[i];

    return ParamType::ParamTypeMax;
}

Component::ParamTag Component::getParamTag(void *param) const
{
    for (int i = 0; i < numParams; i++)
        if (params[i] == param)
            return paramTags[i];

    return ParamTag::TagNone;
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
