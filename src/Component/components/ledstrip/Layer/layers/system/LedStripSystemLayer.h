#pragma once

class LedStripSystemLayer : public LedStripLayer
{
public:
    LedStripSystemLayer(LedStripComponent *strip) : LedStripLayer("systemLayer", LedStripLayer::System, strip) {}
    ~LedStripSystemLayer() {}

    long timeAtStartup = 0;

    DeclareBoolParam(showBattery, false);

    void setupInternal(JsonObject o) override;
    void updateInternal() override;
    void clearInternal() override;

    void showBatteryStatus();
    void updateConnectionStatus();
    void updateShutdown();


    HandleSetParamInternalStart
        HandleSetParamInternalMotherClass(LedStripLayer);
        CheckAndSetParam(showBattery);
    HandleSetParamInternalEnd;

    FillOSCQueryInternalStart
        FillOSCQueryInternalMotherClass(LedStripLayer);
        FillOSCQueryBoolParam(showBattery);
    FillOSCQueryInternalEnd;
};