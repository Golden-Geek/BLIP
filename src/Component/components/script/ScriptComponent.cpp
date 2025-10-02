ImplementSingleton(ScriptComponent);

void ScriptComponent::setupInternal(JsonObject o)
{
    AddStringParamConfig(scriptAtLaunch);
}

bool ScriptComponent::initInternal()
{
    bool result = script.init();

    if(result)
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
    else if (CheckCommand("setScriptParam", 2))
    {
        script.setScriptParam(data[0].intValue(), (float)data[1]);
        return true;
    }

    return false;
}