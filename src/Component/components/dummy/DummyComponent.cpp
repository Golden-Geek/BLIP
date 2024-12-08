ImplementManagerSingleton(Dummy)

void DummyComponent::setupInternal(JsonObject o)
{
    AddBoolParam(param1);
    AddBoolParam(param2);
    AddIntParam(param3);
    AddFloatParam(param4);
    AddStringParam(param5);
}