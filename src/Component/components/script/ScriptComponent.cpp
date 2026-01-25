#include "UnityIncludes.h"
#include "ScriptComponent.h"

ImplementSingleton(ScriptComponent);

void ScriptComponent::setupInternal(JsonObject o)
{
    script.localComponent = this;

    AddStringParamConfig(scriptAtLaunch);
    AddIntParam(universe);
    AddIntParam(startChannel);

#if defined USE_DMX || defined USE_ARTNET
    DMXReceiverComponent::instance->registerDMXListener(this);
#endif
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
#if defined USE_DMX || defined USE_ARTNET
    DMXReceiverComponent::instance->unregisterDMXListener(this);
#endif
    script.shutdown();
}

bool ScriptComponent::handleCommandInternal(const std::string &command, var *data, int numData)
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

void ScriptComponent::sendScriptEvent(std::string eventName)
{
    var data[1];
    data[0] = eventName;
    sendEvent(ScriptEvent, data, 1);
}

void ScriptComponent::sendScriptParamFeedback(std::string paramName, float value)
{
    var data[2];
    data[0] = paramName;
    data[1] = value;
    sendEvent(ScriptParamFeedback, data, 2);
}

void ScriptComponent::onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len)
{
    if (!script.isRunning)
        return;

    if (universe == this->universe)
    {
        int dataStartIndex = this->startChannel - startChannel;

        if (dataStartIndex < 0 || dataStartIndex + WASM_VARIABLES_MAX >= len)
            return;

        script.setParamsFromDMX(data + dataStartIndex, len);
    }
}