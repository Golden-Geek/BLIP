#ifdef USE_FX
class FXComponent : public Component
{
public:
    FXComponent(LedStripComponent* strip) : Component(getTypeString(), false), strip(strip)
    {
    }

    LedStripComponent* strip;

    float curTime = 0;
    float prevTime = 0;

    DeclareFloatParam(staticOffset, 0);
    DeclareFloatParam(offsetSpeed, 0);

    DeclareFloatParam(isolationSpeed, 0)
        DeclareFloatParam(isolationSmoothing, 0);
    float prevIsolationAngle = 0;

    enum IsoAxis
    {
        ProjectedAngle,
        Yaw,
        Pitch,
        Roll,
        IsoAxisMax
    };
    const std::string isoOptions[IsoAxisMax]{"Projected Angle", "Yaw", "Pitch", "Roll"};

    DeclareEnumParam(isolationAxis, ProjectedAngle); // 0 = projectedAngle, 1 = yaw, 2 = pitch, 3 = roll

    DeclareBoolParam(swapOnFlip, false); // this is a hack to make the flip work with the current implementation of the isolation axis, using quaternions should be better for that
    bool boardIsFlipped = false;
    int flipFrameCount = 0;
    int flipDebounce = 0;

    DeclareBoolParam(showCalibration, false);

    Color colors[LED_MAX_COUNT];

    virtual void setupInternal(JsonObject o) override;
    virtual void updateInternal() override;
    virtual void clearInternal() override;

    void process(Color* sourcColors);
    void reset();

    std::string getTypeString() const override { return "fx"; }
};

#endif