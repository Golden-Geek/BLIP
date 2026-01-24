ImplementManagerSingleton(Button);

void ButtonComponent::setupInternal(JsonObject o)
{
    // value.val.type = 'b'; //force boolean
    // SetParam(value,false);

    mode = BUTTON_DEFAULT_MODE;
    if (index == 0)
    {
        pin = BUTTON_DEFAULT_PIN;
        inverted = BUTTON_DEFAULT_INVERTED;
    }

    IOComponent::setupInternal(o);

    debounceCount = 0;
    timeAtPress = 0;

    AddIntParamFeedback(multiPressCount);
    // multiPressCount.readOnly = true;

    AddBoolParamFeedback(longPress);
    // longPress.readOnly = true;

    AddBoolParamFeedback(veryLongPress);

    AddBoolParamConfig(canShutDown);
}

void ButtonComponent::updateInternal()
{
    IOComponent::updateInternal();

    if (value)
    {
        if (millis() < 1000 && !wasPressedAtBoot)
        {
            wasPressedAtBoot = true;
            NDBG("Button was pressed at boot");
        }

        if (!longPress && millis() > timeAtPress + LONGPRESS_TIME)
        {
            SetParam(longPress, true);
        }

        if (!veryLongPress && millis() > timeAtPress + VERYLONGPRESS_TIME)
        {
            SetParam(veryLongPress, true);
        }
    }
    else
    {
        if (wasPressedAtBoot && !releasedAfterBootPress)
        {
            releasedAfterBootPress = true;
            NDBG("Button was released after boot press");
        }
    }

    if (millis() > timeAtPress + MULTIPRESS_TIME)
    {
        if (multiPressCount > 0)
        {
            SetParam(multiPressCount, 0);
        }
    }
}

void ButtonComponent::paramValueChangedInternal(void *param)
{
    IOComponent::paramValueChangedInternal(param);

    if (param == &value)
    {
        // NDBG("Reset long press and very long press");
        SetParam(veryLongPress, false);
        SetParam(longPress, false);

        if (value)
        {
            timeAtPress = millis();
            SetParam(multiPressCount, multiPressCount + 1);
        }
        else
        {
            if (millis() < timeAtPress + SHORTPRESS_TIME)
            {
                // sendEvent(ShortPress); //should be trigger
            }
        }
    }
}
