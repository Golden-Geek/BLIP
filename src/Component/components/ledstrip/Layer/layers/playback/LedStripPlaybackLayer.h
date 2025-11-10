#pragma once

#ifndef PLAYBACK_MAX_SCRIPTS
#define PLAYBACK_MAX_SCRIPTS 5
#endif

class LedStripPlaybackLayer : public LedStripLayer
{
public:
    LedStripPlaybackLayer(LedStripComponent *strip) : LedStripLayer("playbackLayer", LedStripLayer::Bake, strip) {}
    ~LedStripPlaybackLayer() {}

    File curFile;
    File metaDataFile;
    String curFilename;

    int frameSize;

    // info
    int fps;
    float totalTime;
    long totalFrames;
    int groupID;
    int localID;
    Color groupColor;

    // control
    DeclareBoolParam(idMode, false);
    DeclareBoolParam(loop, false);

    // playing
    bool isPlaying;
    long curTimeMs;
    long startTimeMs;
    long prevTimeMs;
    long timeSinceLastSeek;
    float timeToSeek; // used to limit seeking
    bool resyncPending;

#ifdef USE_SCRIPT
    int numScripts;
    String scripts[PLAYBACK_MAX_SCRIPTS];
    float scriptStartTimes[PLAYBACK_MAX_SCRIPTS];
    float scriptEndTimes[PLAYBACK_MAX_SCRIPTS];
    int activeScriptIndex;
#endif

    void setupInternal(JsonObject o) override;
    void updateInternal() override;
    void clearInternal() override;

    bool playFrame();
    void showBlackFrame();
    void showIdFrame();
    void showCurrentFrame();

    void playScripts();

    // play control
    void load(String path, bool force = false);
    void play(float atTime = 0);
    void seek(float t, bool doSendEvent = true);
    void pause();
    void stop();
    void togglePlayPause();

    void onEnabledChanged() override;
    void paramValueChangedInternal(void *param) override;

    bool handleCommandInternal(const String &command, var *data, int numData) override;

    // Time computation helpers
    int64_t msToBytePos(int64_t timeMs) const { return msToFrame(timeMs) * (int64_t)frameSize; } // rgba
    int64_t msToFrame(int64_t timeMs) const { return (int64_t)floor((double)timeMs * (double)fps / 1000.0); }
    long frameToMs(long frame) const { return frame * 1000 / fps; }
    float frameToSeconds(long frame) const { return frame * 1.0f / fps; };
    float msToSeconds(long timeMs) const { return timeMs / 1000.0f; }
    int64_t secondsToMs(float s) const { return (int64_t)llround((double)s * 1000.0); }
    long secondsToFrame(float s) const { return s * fps; }
    long bytePosToFrame(int64_t pos) const { return pos / frameSize; }
    long bytePosToMs(int64_t pos) const { return frameToMs(bytePosToFrame(pos)); }
    long bytePosToSeconds(int64_t pos) const { return frameToSeconds(bytePosToFrame(pos)); }

    DeclareComponentEventTypes(Loaded, LoadError, Playing, Paused, Stopped, Seek, Looped);
    DeclareComponentEventNames("Loaded", "LoadError", "Playing", "Paused", "Stopped", "Seek", "Looped");

    HandleSetParamInternalStart
        HandleSetParamInternalMotherClass(LedStripLayer)
            CheckAndSetParam(idMode);
    CheckAndSetParam(loop);
    HandleSetParamInternalEnd;

    FillSettingsInternalStart
        FillSettingsInternalMotherClass(LedStripLayer)
            FillSettingsParam(idMode);
    FillSettingsParam(loop);
    FillSettingsInternalEnd;

    FillOSCQueryInternalStart
        FillOSCQueryInternalMotherClass(LedStripLayer)
            FillOSCQueryBoolParam(idMode);
    FillOSCQueryBoolParam(loop);
    FillOSCQueryInternalEnd
};