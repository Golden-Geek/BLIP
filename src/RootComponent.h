#pragma once

DeclareComponentSingleton(Root, "root", )

    bool remoteWakeUpMode = false;

// system
SettingsComponent settings;

#ifdef USE_WIFI
WifiComponent wifi;
#endif

CommunicationComponent comm;

#ifdef USE_DISPLAY
DisplayComponent display;
#endif

#ifdef USE_FILES
FilesComponent files;
#endif

#ifdef USE_BATTERY
BatteryComponent battery;
#endif

#ifdef USE_SEQUENCE
SequenceComponent sequence;
#endif

#if defined USE_DMX || defined USE_ARTNET
DMXReceiverComponent dmxReceiver;
#endif

#ifdef USE_LEDSTRIP
LedStripManagerComponent strips;
#endif

#ifdef USE_PWMLED
PWMLedManagerComponent pwmleds;
#endif

#ifdef USE_SCRIPT
ScriptComponent script;
#endif

#ifdef USE_IO
IOManagerComponent ios;
#endif

#ifdef USE_BUTTON
ButtonManagerComponent buttons;
#endif

#ifdef USE_IR
IRComponent ir;
#endif

#ifdef USE_BEHAVIOUR
BehaviourManagerComponent behaviours;
#endif

#ifdef USE_DUMMY
DummyManagerComponent dummies;
#endif

#ifdef USE_MOTION
MotionComponent motion;
#endif

#ifdef USE_SERVO
ServoComponent servo;
#endif

#ifdef USE_STEPPER
StepperComponent stepper;
#endif

#ifdef USE_DC_MOTOR
DCMotorComponent motor;
#endif

#ifdef USE_DISTANCE
DistanceSensorManagerComponent distances;
#endif

#ifdef USE_DIPSWITCH
DIPSwitchComponent dipswitch;
#endif

//High Priority Components
TaskHandle_t fastTask;

// Behaviour
Timer<5> timer;
unsigned long timeAtStart;
unsigned long timeAtShutdown;
unsigned long timeAtLastSignal = 0;

bool demoMode = false;
int demoIndex = 0;

std::map<std::string, Component*> pathComponentMap;
std::vector<Component*> highPriorityComponents;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
static void fastTaskLoop(void* ctx);

void shutdown(bool restarting = false);
void restart();
void standby();

void reboot();
void powerdown();

void switchToWifi();
void switchToESPNow();

void onChildComponentEvent(const ComponentEvent &e) override;
void childParamValueChanged(Component *caller, Component *comp, void *param);

bool handleCommandInternal(const std::string &command, var *data, int numData) override;

void registerComponent(Component *comp, const std::string &path, bool highPriority = false);
void unregisterComponent(Component *comp, const std::string &path);


bool isShuttingDown() const { return timeAtShutdown > 0; }

EndDeclareComponent