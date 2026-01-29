#include "UnityIncludes.h"
#include "RootComponent.h"

#ifdef USE_POWER
#ifndef POWER_EXT
#ifndef C6_BOARD
#define POWER_EXT 0
#else
#define POWER_EXT 1
#endif // C6_BOARD
#endif // POWER_EXT
#endif // USE_POWER

#ifndef DEMO_MODE_COUNT
#define DEMO_MODE_COUNT 3
#endif

ImplementSingleton(RootComponent);

void RootComponent::setupInternal(JsonObject)
{
    BoardInit;

    exposeEnabled = false;
    saveEnabled = false;
    isCritical = true;

    timeAtStart = millis();
    timeAtShutdown = 0;

#ifdef USE_POWER
    if (settings.wakeUpButton > 0)
#if POWER_EXT == 0
        esp_sleep_enable_ext0_wakeup((gpio_num_t)settings.wakeUpButton, settings.wakeUpState);
#else
        esp_sleep_enable_ext1_wakeup(1ULL << settings.wakeUpButton, settings.wakeUpState ? ESP_EXT1_WAKEUP_ANY_HIGH : ESP_EXT1_WAKEUP_ANY_LOW);
#endif
#endif // USE_POWER

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
        comm.init();

        NDBG("Waiting for wake up " + std::to_string(millis() - timeAtStart));
        while (!comm.espNow.wakeUpReceived)
        {
            comm.update();

            if ((millis() - timeAtStart) > 200)
            {
                NDBG("No wake up received");
                standby();
                return;
            }
        }

        NDBG("Wake up received");
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

#if defined USE_DMX || defined USE_ARTNET
    AddOwnedComponent(&dmxReceiver);
#endif

#ifdef USE_IO
    AddOwnedComponent(&ios);
#endif

#ifdef USE_BUTTON
    AddOwnedComponent(&buttons);
#endif

#ifdef USE_IR
    AddOwnedComponent(&ir);
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

#ifdef USE_DISTANCE
    AddOwnedComponent(&distances);
#endif

#ifdef USE_DIPSWITCH
    AddOwnedComponent(&dipswitch);
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

// SPECIALS
#ifdef USE_FLOWTOYS_CONNECT
    AddOwnedComponent(&flowtoysConnect);
#endif
    //

    AddFunctionTrigger(shutdown);
    AddFunctionTrigger(restart);
    AddFunctionTrigger(standby);
#if defined USE_WIFI && defined USE_ESPNOW
    AddFunctionTrigger(switchToWifi);
    AddFunctionTrigger(switchToESPNow);
#endif
}

bool RootComponent::initInternal()
{
    int fastTaskCore = 1;
#if defined(CONFIG_FREERTOS_UNICORE)
    fastTaskCore = 0;
#elif defined(SOC_CPU_CORES_NUM)
#if SOC_CPU_CORES_NUM == 1
    fastTaskCore = 0;
#endif
#elif defined(ARDUINO_RUNNING_CORE)
    fastTaskCore = ARDUINO_RUNNING_CORE;
#endif
    xTaskCreatePinnedToCore(RootComponent::fastTaskLoop, "FastTask", 10000, this, 1, &fastTask, fastTaskCore);
    return true;
}

void RootComponent::updateInternal()
{
    timer.tick();
}

// Fast task loop for high priority components
void RootComponent::fastTaskLoop(void *ctx)
{
    RootComponent *root = (RootComponent *)ctx;
    for (;;)
    {
        for (auto comp : root->highPriorityComponents)
            comp->update(true);
        vTaskDelay(1);
    }
}

void RootComponent::shutdown(bool restarting)
{
    NDBG("Sleep now, baby.");

    comm.sendMessage(this, "bye", restarting ? "restart" : "shutdown");
    comm.server.sendBye(restarting ? "restart" : "shutdown");

#ifdef USE_LEDSTRIP
    strips.shutdown();
#endif

    timeAtShutdown = millis();

    if (restarting)
    {
        timer.in(SHUTDOWN_ANIM_TIME * 1000, [](void *) -> bool
                 {
                     RootComponent::instance->reboot();
                     return false; });
    }
    else
    {
        timer.in(SHUTDOWN_ANIM_TIME * 1000, [](void *) -> bool
                 {  
                    RootComponent::instance->powerdown(); 
                    return false; });
    }
}

void RootComponent::restart()
{
    NDBG("Restarting...");
    shutdown(true);
}

void RootComponent::standby()
{
    NDBG("Standby !");

    comm.sendMessage(this, "bye", "standby");
    comm.server.sendBye("standby");

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
    delay(200);

#ifdef ESP8266
    ESP.deepSleep(5e6);
#else
    esp_deep_sleep_start();
#endif
}

void RootComponent::switchToWifi()
{
#ifdef USE_ESPNOW
    comm.espNow.setEnabled(false);
    settings.saveSettings();
    restart();
#endif
}

void RootComponent::switchToESPNow()
{
#ifdef USE_ESPNOW
    comm.espNow.setEnabled(true);
    settings.saveSettings();
    restart();
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
            std::string address = e.data[0].stringValue();
#ifdef ESPNOW_BRIDGE
            if (address.starts_with("dev")) // routing message
                return;
#endif

            Component *targetComponent = nullptr;
            if (address == "" || address == "root")
            {
                targetComponent = this;
            }
            else
            {

                for (int i = 0; i < allComponents.size(); i++)
                {
                    if (allComponentPaths[i] == address)
                    {
                        targetComponent = allComponents[i];
                        break;
                    }
                }
            }

            if (targetComponent != nullptr)
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
                comm.server.setupConnection();

#if defined USE_DMX || defined USE_ARTNET
                dmxReceiver.setupConnection();
#endif

#ifdef USE_OSC
                comm.osc.setupConnection();
#endif

#if defined USE_ESPNOW && defined ESPNOW_BRIDGE
                // if (wifi.isUsingWiFi())
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
            NDBG("Critical battery, shutting down");
            shutdown();
        }
    }
#endif

    comm.sendEventFeedback(e);
}

void RootComponent::childParamValueChanged(Component *caller, Component *comp, ParamInfo *paramInfo)
{
    void *param = paramInfo->ptr;
#ifdef USE_BUTTON
    if (caller == &buttons)
    {
        ButtonComponent *bc = (ButtonComponent *)comp;
        // DBG("Root param value changed " + bc->name+" > "+String(param == &bc->veryLongPress) + " / " + std::to_string(bc->veryLongPress)+" can sd : "+String(bc->canShutDown)+" / "+String(buttons.items[0]->canShutDown));
        if (param == &bc->veryLongPress && bc->veryLongPress && bc->canShutDown && (!bc->wasPressedAtBoot || bc->releasedAfterBootPress))
        {
            NDBG("Shutdown from button");
            shutdown();
        }
        else if (param == &bc->value)
        {
#ifdef USE_SCRIPT
            if (demoMode)
            {
                if (bc->value > 0)
                {
                    NDBG("Loading next demo script");

                    const int maxDemoCount = 4;
                    demoIndex = (demoIndex + 1) % maxDemoCount;
                    script.script.load("demo" + std::to_string(demoIndex));
                }
            }
#endif
        }
        else if (param == &bc->multiPressCount)
        {

#ifdef USE_SCRIPT

            if (bc->multiPressCount == DEMO_MODE_COUNT)
            {
                NDBG("Toggle demo mode");
                demoMode = !demoMode;
                demoIndex = 0;
                // script.script.load("demo" + std::to_string(demoIndex));
            }
#endif

#ifdef USE_ESPNOW
#ifndef ESPNOW_BRIDGE
            if (bc->multiPressCount == ESPNOW_PAIRING_PRESSCOUNT)
            {
                SetParam(comm.espNow.pairingMode, !comm.espNow.pairingMode);
            }
            else if (bc->multiPressCount == ESPNOW_ENABLED_PRESSCOUNT)
            {
                comm.espNow.setEnabled(!comm.espNow.enabled);
                settings.saveSettings();
                restart();
            }
#endif
#endif
        }
    }

#endif
}

bool RootComponent::handleCommandInternal(const std::string &command, var *data, int numData)
{
    if (command == "shutdown")
        shutdown();
    else if (command == "restart")
        restart();
    else if (command == "stats")
    {
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t heapSize = ESP.getHeapSize();

        std::string stats = DeviceName +
                            "\nID " + std::to_string(settings.propID) +
                            "\n " + DeviceType +
                            "\nVersion " + settings.firmwareVersion +
                            "\n \"DeviceID\": " + settings.getDeviceID() +
                            "\nHeap Size: " + std::to_string(heapSize / 1024) + " kb, Free Heap: " + std::to_string(freeHeap / 1024) + " kb  (" + std::to_string(freeHeap * 100 / heapSize) + "%)" +
                            "\n Min Free Heap: " + std::to_string(ESP.getMinFreeHeap() / 1024) + " kb " +
                            "\n Max Alloc Heap: " + std::to_string(ESP.getMaxAllocHeap() / 1024) + " kb";

#ifdef USE_FILES
        stats += "\n" + files.getFileSystemInfo();
#endif

        comm.sendMessage(this, "stats", stats);
    }
    else if (command == "log")
    {
        NDBG("Logging " + std::to_string(numData) + " values");
        for (int i = 0; i < numData; i++)
            NDBG("> " + data[i].stringValue());
    }
    else
    {
        return false;
    }
    return true;
}

void RootComponent::registerComponent(Component *comp, const std::string &path, bool highPriority)
{
    // DBG("Register component " + path);
    allComponents.push_back(comp);
    allComponentPaths.push_back(path);
    if (highPriority)
        highPriorityComponents.push_back(comp);
}

void RootComponent::unregisterComponent(Component *comp)
{
    int index = -1;
    for (size_t i = 0; i < allComponents.size(); i++)
    {
        if (allComponents[i] == comp)
        {
            index = i;
            break;
        }
    }

    if (index >= 0)
    {
        allComponents.erase(allComponents.begin() + index);
        allComponentPaths.erase(allComponentPaths.begin() + index);
    }

    if (highPriorityComponents.size() > 0)
    {
        auto it = std::find(highPriorityComponents.begin(), highPriorityComponents.end(), comp);
        if (it != highPriorityComponents.end())
            highPriorityComponents.erase(it);
    }
}
