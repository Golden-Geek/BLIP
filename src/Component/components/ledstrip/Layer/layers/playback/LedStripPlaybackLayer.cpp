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
    timeSinceLastSeek = 0;
    timeToSeek = -1;

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
    if (curFile.available() < frameSize)
    {
        NDBG("End of show");
        if (loop)
        {
            sendEvent(Looped);
            play(0);
        }
        else
        {
            stop();
        }
    }

    long mil = millis();
    curTimeMs += mil - prevTimeMs;
    prevTimeMs = mil;

    long fPos = curFile.position();
    long pos = msToBytePos(curTimeMs);

    if (pos < 0)
        return false;
    if (pos < fPos)
        return false; // waiting for frame

    playScripts();

    int skippedFrames = 0;
    while (fPos < pos)
    {
        skippedFrames++;
        curFile.read((uint8_t *)colors, frameSize);
        fPos = curFile.position();
        if (curFile.available() < frameSize)
        {
            DBG("Player overflowed, should not be here");
            return false;
        }
    }

    if (skippedFrames > 0)
    {
        DBG("Skipped frame " + String(skippedFrames));
    }

    if (fPos != pos)
    {
        DBG("Error, position is " + String(fPos) + ", expected " + String(pos));
    }

    curFile.read((uint8_t *)colors, frameSize);

    // showCurrentFrame();
    return true;
}

void LedStripPlaybackLayer::showBlackFrame()
{
    clearColors();
}

void LedStripPlaybackLayer::showIdFrame()
{
    NDBG("Show id frame " + String(groupID) + ":" + String(localID));
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

    // DBG("Play Script " + String(curT));
    if (activeScriptIndex != -1)
    {
        // DBG("Active script index " + String(activeScriptIndex));
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
            // DBG("Check for script " + String(scriptStartTimes[i]));
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

void LedStripPlaybackLayer::load(String path, bool force)
{
    if (path == curFilename && !force)
        return;

#ifdef USE_FILES
    showBlackFrame();

    const String playbackDir = "/playback";

    NDBG("Load file " + path);
    NDBG("Reading meta data");
    DynamicJsonDocument metaData(1000);
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
            NDBG(String("DeserializeJson() failed: ") + String(error.c_str()) + "\nMeta content : " + String(metaDataFile.readString()));
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
                scripts[i] = metaData["scripts"][i]["name"].as<String>();
                scriptStartTimes[i] = (float)metaData["scripts"][i]["start"];
                scriptEndTimes[i] = (float)metaData["scripts"][i]["end"];
            }
#endif

            NDBG("Loaded meta, id " + String(groupID) + ":" + String(localID) + " at " + String(fps) + " fps, "
#ifdef USE_SCRIPT
                 + String(numScripts) + " scripts"
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
        NDBG("File loaded, " + String(totalBytes) + " bytes" + ", " + String(totalFrames) + " frames, " + String(totalTime) + " time");

        if (idMode)
            showIdFrame();
    }

    curFilename = path;

#endif
}

void LedStripPlaybackLayer::play(float atTime)
{
    if (!curFile)
        return;

    isPlaying = true;

    NDBG("Play here ar time " + String(atTime));

    seek(atTime, false);
    prevTimeMs = millis();

    sendEvent(Playing);
}

void LedStripPlaybackLayer::seek(float t, bool doSendEvent)
{
    if (!curFile)
        return;

    // NDBG("Seek to " + String(t) + "s");

    curTimeMs = secondsToMs(t);
    prevTimeMs = millis();

    curFile.seek(msToBytePos(max(curTimeMs, (long)0)));

    if (curTimeMs < 0)
    {
        showBlackFrame();
    }
    else if (!isPlaying)
    {
        curFile.read((uint8_t *)colors, frameSize);
    }

    timeToSeek = -1;

    if (doSendEvent)
        sendEvent(Seek);
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
        stop();
        showBlackFrame();
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
        NDBG("ID Mode Switch " + String(idMode));
        if (idMode)
            showIdFrame();
        else
            seek(msToSeconds(curTimeMs));
    }
}

bool LedStripPlaybackLayer::handleCommandInternal(const String &command, var *data, int numData)
{
    LedStripLayer::handleCommandInternal(command, data, numData);

    if (checkCommand(command, "load", numData, 1))
    {
        load(data[0].stringValue());
        return true;
    }

    if (checkCommand(command, "play", numData, 0))
    {
        if (numData > 0 && data[0].type == 's')
        {
            String path = data[0].stringValue();
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
            NDBG("Play sync " + data[0].stringValue() + " at " + String(data[1].floatValue()) + "s");
            SetParam(enabled, true);
            String path = data[0].stringValue();
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

    return false;
}