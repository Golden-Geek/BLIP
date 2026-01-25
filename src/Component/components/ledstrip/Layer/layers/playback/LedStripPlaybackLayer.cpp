#include "UnityIncludes.h"

void LedStripPlaybackLayer::setupInternal(JsonObject o)
{
    LedStripLayer::setupInternal(o);

#ifdef PLAYBACK_USE_ALPHA
    frameSize = strip->numColors * 4;
#else
    frameSize = strip->numColors * 3;
#endif

    fps = 0;
    totalTime = 0;
    totalFrames = 0;
    groupID = -1;
    localID = -1;
    AddBoolParam(idMode);
    AddBoolParam(loop);
    isPlaying = false;
    curTimeMs = 0;
    prevTimeMs = 0;
    startTimeMs = 0;
    timeSinceLastSeek = 0;
    timeToSeek = -1;
    resyncPending = false;

#ifdef USE_SCRIPT
    activeScriptIndex = -1;
#endif
}

void LedStripPlaybackLayer::updateInternal()
{
    if (!curFile)
    {
        return;
    }

    if (idMode)
    {
        // showIdFrame();
        return;
    }

    if (timeToSeek != -1 && millis() > timeSinceLastSeek + 20)
    {
        seek(timeToSeek);
        timeToSeek = -1;
        timeSinceLastSeek = millis();
    }

    if (isPlaying)
        playFrame();
}

void LedStripPlaybackLayer::clearInternal()
{
}

bool LedStripPlaybackLayer::playFrame()
{
    if (!isPlaying || !curFile)
        return false;

    // 1) End-of-file handling
    if (curFile.available() < frameSize)
    {
        NDBG("End of show");
        if (loop)
        {
            sendEvent(Looped);
            play(0.0f);
        }
        else
        {
            stop();
        }
        return false;
    }

    // 2) Absolute timing → target frame index
    const double fps_d = (double)fps; // ensure float math
    const unsigned long now = millis();
    const unsigned long elapsedMs = now - startTimeMs; // startTimeMs set in play()
    if (fps_d <= 0.0)
        return false;

    // target frame using floor to avoid oscillation
    const int64_t targetFrame = (int64_t)floor((elapsedMs * fps_d) / 1000.0);
    const int64_t desiredPos = targetFrame * (int64_t)frameSize;

    // 3) Read current file position and align sanity
    int64_t currentPos = curFile.position();

    // Force a one-time hard snap right after a seek during playback
    if (resyncPending)
    {
        if (!curFile.seek(desiredPos))
        {
            DBG("resync seek() failed to " + std::to_string((long)desiredPos));
            return false;
        }
        resyncPending = false;
        // update currentPos to the new location
        // (optional realign if needed)
    }

    // (Optional) snap obviously off-boundary positions down to nearest frame boundary
    if (currentPos % frameSize != 0)
    {
        const int64_t aligned = (currentPos / frameSize) * (int64_t)frameSize;
        curFile.seek(aligned);
        currentPos = curFile.position();
    }

    // 4) Compute drift in bytes (+ ahead, - behind)
    const int64_t delta = currentPos - desiredPos;

    // thresholds (tune as needed)
    const int64_t ONE_FRAME = (int64_t)frameSize;
    const int64_t MAX_LAX = ONE_FRAME;        // within ±1 frame → wait/skip without error
    const int64_t FORCE_SEEK = ONE_FRAME * 2; // ≥2 frames → snap with seek

    // 5) Correct big drifts with seek()
    if (llabs(delta) >= FORCE_SEEK)
    {
        if (!curFile.seek(desiredPos))
        {
            DBG("seek() failed to " + std::to_string((long)desiredPos));
            return false;
        }
        currentPos = curFile.position();
        // After successful seek, treat as perfectly aligned
    }
    else
    {
        // 6) Small drift handling without errors
        if (delta > 0 && delta <= MAX_LAX)
        {
            // We are up to one frame AHEAD of time → wait until time catches up
            // (do not read; let scheduler call us again soon)
            return false;
        }
        else if (delta < 0 && -delta <= MAX_LAX)
        {
            // We are up to one frame BEHIND → skip one frame to catch up
            if (curFile.available() >= frameSize)
            {
                uint8_t dummy[1]; // avoid large stack; do an actual seek instead of read
                // Fast-forward by exactly one frame without allocating:
                const int64_t skipTo = currentPos + ONE_FRAME;
                if (!curFile.seek(skipTo))
                {
                    DBG("skip seek() failed");
                    return false;
                }
                currentPos = curFile.position();
            }
            else
            {
                return false; // nothing to read yet
            }
        }
        // else delta == 0 → perfectly aligned
    }

    // 7) Final alignment check (only warn if still off by weird amount)
    currentPos = curFile.position();
    const int64_t finalDelta = currentPos - desiredPos;
    if (finalDelta != 0 && llabs(finalDelta) != ONE_FRAME)
    {
        // Only log occasionally to avoid spam
        DBG("Position mismatch tolerated: file=" + std::to_string((long)currentPos) +
            ", expected=" + std::to_string((long)desiredPos) +
            ", delta=" + std::to_string((long)finalDelta));
        // Continue anyway (don’t hard fail)
    }

    // 8) Ensure we have a full frame buffered
    if (curFile.available() < frameSize)
    {
        // Small underrun — try again on next tick
        return false;
    }

    // 9) Read the frame
    size_t n = curFile.read((uint8_t *)colors, frameSize);
    if (n != (size_t)frameSize)
    {
        DBG("Short read: " + std::to_string((long)n) + " of " + std::to_string(frameSize));
        return false;
    }

    // 10) Trigger per-frame scripts AFTER the read (so visuals are up to date)
    playScripts();
    return true;
}

void LedStripPlaybackLayer::showBlackFrame()
{
    clearColors();
}

void LedStripPlaybackLayer::showIdFrame()
{
    NDBG("Show id frame " + std::to_string(groupID) + ":" + std::to_string(localID));
    if (groupID == -1 || localID == -1)
        return;

    fillRange(groupColor, 1 - 3.f / strip->numColors, 1);
    Color c = Color::HSV(localID * 1.0f / 4, 1, 1);
    fillRange(c, 0, localID * 1.f / strip->numColors, false);
}

void LedStripPlaybackLayer::playScripts()
{
#ifdef USE_SCRIPT
    float curT = curTimeMs / 1000.0f;
    // float prevT = prevTimeMs / 1000.0f;

    // DBG("Play Script " + std::to_string(curT));
    if (activeScriptIndex != -1)
    {
        // DBG("Active script index " + std::to_string(activeScriptIndex));
        if (scriptEndTimes[activeScriptIndex] < curT)
        {
            // NDBG("Start script");
            ScriptComponent::instance->script.stop();
            activeScriptIndex = -1;
        }
    }
    else
    {
        for (int i = 0; i < numScripts; i++)
        {
            // DBG("Check for script " + std::to_string(scriptStartTimes[i]));
            if (curT >= scriptStartTimes[i] && curT < scriptEndTimes[i])
            {
                // NDBG("Stop script");
                ScriptComponent::instance->script.load(scripts[i]);
                activeScriptIndex = i;
                break;
            }
        }
    }
#endif
}

void LedStripPlaybackLayer::load(const std::string& path, bool force)
{
    if (path == curFilename && !force)
        return;

#ifdef USE_FILES
    showBlackFrame();

    const std::string playbackDir = "/playback";

    NDBG("Load file " + path);
    NDBG("Reading meta data");
    StaticJsonDocument<1000> metaData;
    metaDataFile = FilesComponent::instance->openFile(playbackDir + "/" + path + ".meta", false); // false is for reading
    if (!metaDataFile)
    {
        NDBG("Error reading metadata");
    }
    else
    {
        DeserializationError error = deserializeJson(metaData, metaDataFile);
        if (error)
        {
            NDBG("DeserializeJson() failed: " + std::string(error.c_str()) + "\nMeta content : " + metaDataFile.readString().c_str());
        }
        else
        {
            fps = metaData["fps"];
            groupID = metaData["group"];
            localID = metaData["id"];

            groupColor = Color((float)(metaData["groupColor"][0]) * 255,
                               (float)(metaData["groupColor"][1]) * 255,
                               (float)(metaData["groupColor"][2]) * 255);

#ifdef USE_SCRIPT
            numScripts = metaData["scripts"].size();
            for (int i = 0; i < numScripts; i++)
            {
                scripts[i] = metaData["scripts"][i]["name"].as<std::string>();
                scriptStartTimes[i] = (float)metaData["scripts"][i]["start"];
                scriptEndTimes[i] = (float)metaData["scripts"][i]["end"];
            }
#endif

            NDBG("Loaded meta, id " + std::to_string(groupID) + ":" + std::to_string(localID) + " at " + std::to_string(fps) + " fps, "
#ifdef USE_SCRIPT
                 + std::to_string(numScripts) + " scripts"
#endif
            );
        }

        metaDataFile.close();
    }

    NDBG("Loading colors..");
    curFile = FilesComponent::instance->openFile(playbackDir + "/" + path + ".colors", false); // false is for reading
    if (!curFile)
    {
        NDBG("Error playing file " + path);
        sendEvent(LoadError);
    }
    else
    {
        long totalBytes = curFile.size();
        totalFrames = bytePosToFrame(totalBytes);
        totalTime = bytePosToSeconds(totalBytes);
        curTimeMs = 0;
        isPlaying = false;
        NDBG("File loaded, " + std::to_string(totalBytes) + " bytes" + ", " + std::to_string(totalFrames) + " frames, " + std::to_string(totalTime) + " time");

        if (idMode)
            showIdFrame();
    }

    curFilename = path;

#endif
}

void LedStripPlaybackLayer::unload()
{
    stop();
    if (curFile)
    {
        curFile.close();
        NDBG("File " + curFilename + " unloaded");
    }
    curFilename = "";
    showBlackFrame();
}

void LedStripPlaybackLayer::play(float atTime)
{
    if (!curFile)
        return;

    isPlaying = true;
    NDBG("Play at time " + std::to_string(atTime));

    // time reference
    curTimeMs = (int64_t)(atTime * 1000.0f);
    startTimeMs = millis() - (unsigned long)curTimeMs;

    // seek to exact frame boundary for start
    int64_t startPos = msToFrame(curTimeMs) * (int64_t)frameSize; // floor via msToFrame
    // snap to boundary just in case
    startPos = (startPos / frameSize) * (int64_t)frameSize;

    if (!curFile.seek(startPos))
    {
        DBG("Initial seek failed to " + std::to_string((long)startPos));
        isPlaying = false;
        return;
    }

    prevTimeMs = millis();
    sendEvent(Playing);
}

void LedStripPlaybackLayer::seek(float t, bool doSendEvent)
{
    if (!curFile)
        return;

    // 1) Target playback time (can be negative to show black)
    curTimeMs = secondsToMs(t);

    // 2) Keep absolute-time reference consistent with the new time
    //    This is the key piece that makes mid-play seeks work.
    startTimeMs = millis() - (unsigned long)(curTimeMs > 0 ? curTimeMs : 0);

    // 3) Compute target byte position on a frame boundary
    int64_t targetPos;
    if (curTimeMs < 0)
    {
        targetPos = 0; // before start → clamp to file start
    }
    else
    {
        targetPos = msToBytePos(curTimeMs);
        // snap to exact frame boundary (paranoia)
        targetPos = (targetPos / (int64_t)frameSize) * (int64_t)frameSize;
    }

    // 4) Seek file
    if (!curFile.seek(targetPos))
    {
        DBG("seek() failed to " + std::to_string((long)targetPos));
        return;
    }

    // 5) Visual state for negative times
    if (curTimeMs < 0)
    {
        showBlackFrame();
    }
    else if (!isPlaying)
    {
        // If paused/stopped, optionally prefetch current frame so 'colors' matches the seek position
        if (curFile.available() >= frameSize)
        {
            curFile.read((uint8_t *)colors, frameSize);
            // step back one frame so next playFrame will read the same frame again if desired
            int64_t back = curFile.position() - (int64_t)frameSize;
            if (back >= 0)
                curFile.seek(back);
        }
    }
    else
    {
        // We are seeking while playing: request a hard resync on next tick
        resyncPending = true;
    }

    prevTimeMs = millis(); // not strictly necessary with absolute timing, but harmless

    if (doSendEvent)
        sendEvent(Seek);
    // If you also want an external "PositionChanged" event, fire it here.
}

void LedStripPlaybackLayer::pause()
{
    isPlaying = false;
    sendEvent(Paused);
}

void LedStripPlaybackLayer::stop()
{
    isPlaying = false;
    curTimeMs = 0;
    prevTimeMs = 0;
// showBlackFrame();
#ifdef USE_SCRIPT
    activeScriptIndex = -1;
#endif

    sendEvent(Stopped);
}

void LedStripPlaybackLayer::togglePlayPause()
{
    if (!isPlaying)
        pause();
    else
        play();
}

void LedStripPlaybackLayer::onEnabledChanged()
{
    LedStripLayer::onEnabledChanged();
    if (!enabled)
    {
        unload();
    }
    else
    {
        if (idMode)
            showIdFrame();
    }
}

void LedStripPlaybackLayer::paramValueChangedInternal(void *param)
{
    LedStripLayer::paramValueChangedInternal(param);
    if (param == &idMode)
    {
        NDBG("ID Mode Switch " + std::to_string(idMode));
        if (idMode)
            showIdFrame();
        else
            seek(msToSeconds(curTimeMs));
    }
}

bool LedStripPlaybackLayer::handleCommandInternal(const std::string &command, var *data, int numData)
{
    LedStripLayer::handleCommandInternal(command, data, numData);

    if (checkCommand(command, "load", numData, 1))
    {
        load(data[0].stringValue(), numData > 1 ? (bool)data[1].boolValue() : false);
        return true;
    }

    if (checkCommand(command, "play", numData, 0))
    {
        if (numData > 0 && data[0].type == 's')
        {
            std::string path = data[0].stringValue();
            if (curFilename == path && isPlaying)
            {
                if (numData > 1)
                {
                    timeToSeek = data[1].floatValue();
                }
            }
            else
            {
                load(path);
                play(numData > 1 ? data[1].floatValue() : 0);
            }
        }
        else
        {
            play(numData > 0 ? data[0].floatValue() : 0);
        }

        return true;
    }

    if (checkCommand(command, "playSync", numData, 2))
    {
        // NDBG("Received playSync command");
        if (!isPlaying)
        {
            NDBG("Play sync " + data[0].stringValue() + " at " + std::to_string(data[1].floatValue()) + "s");
            SetParam(enabled, true);
            std::string path = data[0].stringValue();
            float time = data[1].floatValue();
            load(path);
            play(time);
        }
        else
        {
            // NDBG("Already playing, ignore playSync");
        }

        return true;
    }

    if (checkCommand(command, "pause", numData, 0))
    {
        pause();
        return true;
    }

    if (checkCommand(command, "resume", numData, 0))
    {
        play();
        return true;
    }

    if (checkCommand(command, "stop", numData, 0))
    {
        stop();
        return true;
    }

    if (checkCommand(command, "seek", numData, 1))
    {
        timeToSeek = data[0].floatValue();
        return true;
    }
    if (checkCommand(command, "unload", numData, 0))
    {
        unload();
        return true;
    }

    return false;
}