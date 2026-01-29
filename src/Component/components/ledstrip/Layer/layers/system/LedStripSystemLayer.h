#pragma once

class LedStripSystemLayer : public LedStripLayer
{
public:
    LedStripSystemLayer(LedStripComponent *strip) : LedStripLayer("systemLayer", LedStripLayer::System, strip) {}
    ~LedStripSystemLayer() {}

    long timeAtStartup = 0;

    DeclareBoolParam(showBattery, false);
    DeclareColorParam(espSyncColor, 0, 1, 1, 1);

    void setupInternal(JsonObject o) override;
    void updateInternal() override;
    void clearInternal() override;

    void updateCriticalStatus();
    void showBatteryStatus();
    void updateConnectionStatus();
    void updateShutdown();

};