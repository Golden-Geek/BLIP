#include "UnityIncludes.h"
#include "RootComponent.h"

#ifdef USE_POWER
#ifndef POWER_EXT
#define POWER_EXT 0
#endif
#endif

#ifndef TESTMODE_PRESSCOUNT
#define TESTMODE_PRESSCOUNT -1
#endif

ImplementSingleton(RootComponent);

bool RootComponent::availablePWMChannels[16] = {true};

void RootComponent::setupInternal(JsonObject)
{
    BoardInit;

    exposeEnabled = false;
    saveEnabled = false;

    timeAtStart = millis();
    timeAtShutdown = 0;

#ifdef USE_POWER
    if (settings.wakeUpButton > 0)
#if POWER_EXT == 0
        esp_sleep_enable_ext0_wakeup((gpio_num_t)settings.wakeUpButton, settings.wakeUpState);
#else
        esp_sleep_enable_ext1_wakeup(1ULL << settings.wakeUpButton, settings.wakeUpState ? ESP_EXT1_WAKEUP_ANY_HIGH : ESP_EXT1_WAKEUP_ANY_LOW);
#endif
#endif

    // parameters.clear(); // remove enabled in root component
    Settings::loadSettings();
    JsonObject o = Settings::settings.as<JsonObject>();

    AddOwnedComponent(&comm);

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
    {
        remoteWakeUpMode = true;
    }

    if (remoteWakeUpMode)
    {
        // pinMode(18, OUTPUT);
        // digitalWrite(18, HIGH);

        comm.init();

        DBG("Waiting for wake up " + String(millis() - timeAtStart));
        while (!comm.espNow.wakeUpReceived)
        {
            comm.update();

            if ((millis() - timeAtStart) > 200)
            {
                // pinMode(18, OUTPUT);
                // digitalWrite(18, LOW);
                DBG("No wake up received");
                standby();
                return;
            }
        }

        DBG("Wake up received");
        ESP.restart();
    }
#endif
#endif

    AddOwnedComponent(&settings);

#ifdef USE_LEDSTRIP
    AddOwnedComponent(&strips);
#endif

#ifdef USE_WIFI
    AddOwnedComponent(&wifi);
#endif

#ifdef USE_SERVER
    AddOwnedComponent(&server);
#endif


#ifdef USE_DISPLAY
    AddOwnedComponent(&display);
#endif

#ifdef USE_BATTERY
    AddOwnedComponent(&battery);
#endif

#ifdef USE_FILES
    AddOwnedComponent(&files);
#endif

#if USE_SCRIPT
    AddOwnedComponent(&script);
#endif

#ifdef USE_STREAMING
    AddOwnedComponent(&streamReceiver);
#endif

#ifdef USE_IO
    // memset(availablePWMChannels, true, sizeof(availablePWMChannels));
    AddOwnedComponent(&ios);
#ifdef USE_BUTTON
    AddOwnedComponent(&buttons);
#endif
#endif

#ifdef USE_MOTION
    AddOwnedComponent(&motion);
#endif

#ifdef USE_SERVO
    AddOwnedComponent(&servo);
#endif

#ifdef USE_STEPPER
    AddOwnedComponent(&stepper);
#endif

#ifdef USE_DC_MOTOR
    AddOwnedComponent(&motor);
#endif

#ifdef USE_PWMLED
    AddOwnedComponent(&pwmleds);
#endif

#ifdef USE_BEHAVIOUR
    AddOwnedComponent(&behaviours);
#endif

#ifdef USE_DUMMY
    AddOwnedComponent(&dummies);
#endif

#ifdef USE_SEQUENCE
    AddOwnedComponent(&sequence);
#endif
}

void RootComponent::updateInternal()
{
    timer.tick();
}

void RootComponent::shutdown(bool restarting)
{
    NDBG("Sleep now, baby.");

#ifdef USE_LEDSTRIP
    strips.shutdown();
#endif

    timeAtShutdown = millis();

    if (restarting)
    {
        timer.in(1000, [](void *) -> bool
                 { RootComponent::instance->reboot(); return false; });
    }
    else
    {
        timer.in(1000, [](void *) -> bool
                 {  
                RootComponent::instance->powerdown(); return false; });
    }
}

void RootComponent::restart()
{
    DBG("Restarting...");
    shutdown(true);
}



void RootComponent::standby()
{
    DBG("Standby !");
    clear();
    esp_sleep_enable_timer_wakeup(3 * 1000000); // Set wakeup timer for 3 seconds
    esp_deep_sleep_start();
}



void RootComponent::reboot()
{
    clear();
    delay(200);

    ESP.restart();
}

void RootComponent::powerdown()
{
    clear();

    // NDBG("Sleep now, baby.");

    delay(200);

    // #elif defined TOUCH_WAKEUP_PIN
    //     touchAttachInterrupt((gpio_num_t)TOUCH_WAKEUP_PIN, touchCallback, 110);
    //     esp_sleep_enable_touchpad_wakeup();
    // #endif

#ifdef ESP8266
    ESP.deepSleep(5e6);
#else
    esp_deep_sleep_start();
#endif
}

void RootComponent::onChildComponentEvent(const ComponentEvent &e)
{
    if (isShuttingDown())
        return;

    if (e.component == &comm
#ifdef USE_BEHAVIOUR
        || e.component == &behaviours
#endif
    )
    {
        if ((e.component == &comm && e.type == CommunicationComponent::MessageReceived)
#ifdef USE_BEHAVIOUR
            || (e.component == &behaviours && e.type == BehaviourManagerComponent::CommandLaunched)
#endif
        )
        {

            String address = e.data[0].stringValue();
#ifdef ESPNOW_BRIDGE
            if (address.startsWith("dev")) // routing message
                return;
#endif

            if (Component *targetComponent = getComponentWithName(e.data[0].stringValue()))
            {
                bool handled = targetComponent->handleCommand(e.data[1], &e.data[2], e.numData - 2);
                if (!handled)
                    NDBG("Command was not handled " + address + " > " + e.data[1].stringValue());
            }
            else
            {
                NDBG("No component found for " + address);
            }

            return;
        }
    }
#ifdef USE_WIFI
    else if (e.component == &wifi)
    {
        if (e.type == WifiComponent::ConnectionStateChanged)
        {
            if (wifi.state == WifiComponent::Connected)
            {
                NDBG("Setup connections now");
                server.setupConnection();

#ifdef USE_STREAMING
                streamReceiver.setupConnection();
#endif

#ifdef USE_OSC
                comm.osc.setupConnection();
#endif

#if defined USE_ESPNOW && defined ESPNOW_BRIDGE
                comm.espNow.initESPNow();
#endif
            }
        }
    }
#endif

#ifdef USE_BATTERY
    else if (e.component == &battery)
    {
        if (e.type == BatteryComponent::CriticalBattery)
        {
#ifdef USE_LEDSTRIP
            strips.items[0]->setBrightness(.05f);
#endif
            NDBG("Critical battery, shutting down");
            shutdown();
        }
    }
#endif

    comm.sendEventFeedback(e);
}

void RootComponent::childParamValueChanged(Component *caller, Component *comp, void *param)
{
#ifdef USE_BUTTON
    if (caller == &buttons)
    {
        ButtonComponent *bc = (ButtonComponent *)comp;
        // DBG("Root param value changed " + bc->name+" > "+String(param == &bc->veryLongPress) + " / " + String(bc->veryLongPress)+" can sd : "+String(bc->canShutDown)+" / "+String(buttons.items[0]->canShutDown));
        if (param == &bc->veryLongPress && bc->veryLongPress && bc->canShutDown)
        {
            NDBG("Shutdown from button");
            shutdown();
        }
        else if (param == &bc->multiPressCount)
        {
            if (bc->multiPressCount == TESTMODE_PRESSCOUNT)
            {
                NDBG("Toggle testing mode");
                testMode = !testMode;
            }

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
            if (bc->multiPressCount == ESPNOW_PAIRING_PRESSCOUNT)
            {
                SetParam(comm.espNow.pairingMode, !comm.espNow.pairingMode);
            }
            else if (bc->multiPressCount == ESPNOW_ENABLED_PRESSCOUNT)
            {
                comm.espNow.toggleEnabled();
                DBG("Set espNow enabled " + String(comm.espNow.enabled));
                settings.saveSettings();
                restart();
            }
#endif
#endif
        }
    }
#endif
}

bool RootComponent::handleCommandInternal(const String &command, var *data, int numData)
{
    if (command == "shutdown")
        shutdown();
    else if (command == "restart")
        restart();
    else if (command == "stats")
    {
        DBG("Heap " + String(ESP.getFreeHeap()) + " free / " + String(ESP.getHeapSize()) + " total");
        DBG("Free Stack size  " + String((int)uxTaskGetStackHighWaterMark(NULL)) + " free");
        // comm->sendMessage(this, "freeHeap", String(ESP.getFreeHeap()) + " bytes");
    }
    else if (command == "log")
    {
        NDBG("Logging " + String(numData) + " values");
        for (int i = 0; i < numData; i++)
            NDBG("> " + data[i].stringValue());
    }
    else
    {
        return false;
    }

    return true;
}

int RootComponent::getFirstAvailablePWMChannel() const
{
    for (int i = 0; i < 16; i++)
    {
        if (availablePWMChannels[i])
            return i;
    }
    return -1;
}
