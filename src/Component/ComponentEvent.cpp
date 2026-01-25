#include "UnityIncludes.h"

std::string ComponentEvent::getName() const
{
    return component->getComponentEventName(type);
}