#pragma once

class LedStripSystemLayer : public LedStripLayer
{
public:
    LedStripSystemLayer(LedStripComponent *strip) : LedStripLayer("systemLayer", LedStripLayer::System, strip) {}
    ~LedStripSystemLayer() {}

    long timeAtStartup = 0;

    void setupInternal(JsonObject o) override;
    void updateInternal() override;
    void clearInternal() override;

    void showStartupAnimation();
    void updateConnectionStatus();
    void updateShutdown();
};