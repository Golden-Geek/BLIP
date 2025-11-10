#include "UnityIncludes.h"

ImplementSingleton(ScriptComponent);

void ScriptComponent::setupInternal(JsonObject o)
{
    script.localComponent = this;

    AddStringParamConfig(scriptAtLaunch);
}

bool ScriptComponent::initInternal()
{
    bool result = script.init();

    if (result)
    {
        if (scriptAtLaunch.length() > 0)
        {
            script.load(scriptAtLaunch);
        }
    }

    return result;
}

void ScriptComponent::updateInternal()
{
    script.update();
}

void ScriptComponent::clearInternal()
{
    script.shutdown();
}

bool ScriptComponent::handleCommandInternal(const String &command, var *data, int numData)
{
    if (CheckCommand("load", 1))
    {
        script.load(data->stringValue());
        return true;
    }
    else if (CheckCommand("stop", 0))
    {
        script.stop();
        return true;
    }
    else if (CheckCommand("setParam", 2))
    {
        script.setScriptParam(data[0].stringValue(), (float)data[1]);
        return true;
    }
    else if (CheckCommand("trigger", 1))
    {
        script.triggerFunction(data[0].stringValue());
        return true;
    }

    return false;
}

void ScriptComponent::sendScriptEvent(String eventName)
{
    var data[1];
    data[0] = eventName;
    sendEvent(ScriptEvent, data, 1);
}

void ScriptComponent::sendScriptParamFeedback(String paramName, float value)
{
    var data[2];
    data[0] = paramName;
    data[1] = value;
    sendEvent(ScriptParamFeedback, data, 2);
}
