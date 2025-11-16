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

#ifdef USE_STREAMING
LedStreamReceiverComponent streamReceiver;
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

// Behaviour
Timer<5> timer;
unsigned long timeAtStart;
unsigned long timeAtShutdown;
unsigned long timeAtLastSignal = 0;

DeclareBoolParam(testMode, false);
bool demoMode = false;
int demoIndex = 0;

void setupInternal(JsonObject o) override;
void updateInternal() override;


void shutdown(bool restarting = false);
void restart();
void standby();

void reboot();
void powerdown();

void onChildComponentEvent(const ComponentEvent &e) override;
void childParamValueChanged(Component *caller, Component *comp, void *param);

bool handleCommandInternal(const String &command, var *data, int numData) override;

bool isShuttingDown() const { return timeAtShutdown > 0; }

HandleSetParamInternalStart
    CheckTrigger(shutdown);
CheckTrigger(restart);
CheckTrigger(standby);
CheckAndSetParam(testMode);

HandleSetParamInternalEnd;

FillSettingsInternalStart

FillSettingsInternalEnd;

FillOSCQueryInternalStart

    FillOSCQueryTrigger(shutdown);
FillOSCQueryTrigger(restart);
FillOSCQueryTrigger(standby);
FillOSCQueryBoolParam(testMode);

FillOSCQueryInternalEnd

    EndDeclareComponent