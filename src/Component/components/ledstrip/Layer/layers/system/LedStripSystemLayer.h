#pragma once

class LedStripSystemLayer : public LedStripLayer
{
public:
    LedStripSystemLayer(LedStripComponent *strip) : LedStripLayer("systemLayer", LedStripLayer::System, strip) {}
    ~LedStripSystemLayer() {}

    void setupInternal(JsonObject o) override;
    void updateInternal() override;
    void clearInternal() override;

    void updateConnectionStatus();
    void updateShutdown();
};