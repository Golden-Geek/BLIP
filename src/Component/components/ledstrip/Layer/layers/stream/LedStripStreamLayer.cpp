#include "UnityIncludes.h"
#include "LedStripStreamLayer.h"


void LedStripStreamLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);

    AddIntParamConfig(universe);
    AddBoolParamConfig(clearOnNoReception);
    AddFloatParamConfig(noReceptionTime);
}

bool LedStripStreamLayer::initInternal()
{
    LedStreamReceiverComponent::instance->registerLayer(this);

    return true;
}

void LedStripStreamLayer::updateInternal()
{
    if (!hasCleared && clearOnNoReception && millis() / 1000.0f - lastReceiveTime > noReceptionTime)
    {
        clearColors();
        hasCleared = true;
    }
}

void LedStripStreamLayer::clearInternal()
{
    if (LedStreamReceiverComponent::instance != nullptr)
    {
        LedStreamReceiverComponent::instance->unregisterLayer(this);
    }
}

