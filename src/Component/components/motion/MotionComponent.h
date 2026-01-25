#pragma once

#define TRAIL_MAX 20
#define IMU_NATIVE_STACK_SIZE (4 * 1024)

DeclareComponent(Motion, "motion", )

#ifdef IMU_TYPE_BNO055
    Adafruit_BNO055 bno;
#elif defined IMU_TYPE_M5MPU
    m5::IMU_Class mpu;
double lastUpdateTime;
#endif

enum SendLevel
{
    SendLevelNone,
    SendLevelOrientation,
    SendLevelAll,
    SendLevelMax
};
const std::string sendLevelOptions[3]{"None", "Orientation", "All"};

enum ThrowState
{
    ThrowStateNone,
    ThrowStateFlat,
    ThrowStateSingle,
    ThrowStateDouble,
    ThrowStateFlatFront,
    ThrowStateLoftie,
    ThrowStateMax
};
const std::string throwStateOptions[6]{"None", "Flat", "Single", "Double", "Flat Front", "Loftie"};

DeclareBoolParam(connected, false);
DeclareEnumParam(sendLevel, 0);
DeclareIntParam(orientationSendRate, 50);

#ifdef IMU_TYPE_BNO055
DeclareIntParam(sdaPin, IMU_DEFAULT_SDA);
DeclareIntParam(sclPin, IMU_DEFAULT_SCL);
DeclareIntParam(intPin, IMU_DEFAULT_INT);
#endif

long timeSinceOrientationLastSent;

// IMU data
DeclareP3DParam(orientation, 0, 0, 0);
DeclareP3DParam(accel, 0, 0, 0);
DeclareP3DParam(gyro, 0, 0, 0);
DeclareP3DParam(linearAccel, 0, 0, 0);
DeclareFloatParam(orientationXOffset, 0);

DeclareEnumParam(throwState, ThrowStateNone);
DeclareFloatParam(activity, 0);
float prevActivity;
float debug[4];

// IMU Compute
DeclareP2DParam(flatThresholds, .8f, 2);
DeclareP3DParam(accelThresholds, .8f, 2, 4);
DeclareFloatParam(diffThreshold, 8);
DeclareFloatParam(semiFlatThreshold, 2);
DeclareFloatParam(loftieThreshold, 12);
DeclareFloatParam(singleThreshold, 25);

// Projected Angle
DeclareFloatParam(angleOffset, 0);
DeclareFloatParam(projectedAngle, 0);
DeclareFloatParam(xOnCalibration, 0);

// Spin Compute
DeclareIntParam(spinCount, 0);
DeclareFloatParam(spin, 0);

int countNonDouble;
int lastThrowState;
float launchOrientationX;
float launchProjectedAngle;
float currentSpinLastUpdate;

// Threading
bool hasNewData;
bool imuLock;
bool shouldStopRead;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void startIMUTask();

static void readIMUStatic(void *);

bool setupIMU();

void readIMU();
void sendCalibrationStatus();
void computeThrow();
void computeActivity();
void computeProjectedAngle();
void computeSpin();

void setOrientationXOffset(float offset);
void setProjectAngleOffset(float yaw, float angle);

void onEnabledChanged() override;
void paramValueChangedInternal(void *param) override;

bool handleCommandInternal(const std::string &command, var *data, int numData) override;

DeclareComponentEventTypes(OrientationUpdate, AccelUpdate, GyroUpdate, LinearAccelUpdate, ThrowState, CalibrationStatus, ActivityUpdate, Debug, ProjectedAngleUpdate);
DeclareComponentEventNames("orientation", "accel", "gyro", "linearAccel", "throwState", "calibration", "activity", "debug", "projectedAngle");

#ifdef USE_SCRIPT
// LinkScriptFunctionsStart
//     LinkScriptFunctionsStartMotherClass(Component)
//         LinkScriptFunction(MotionComponent, getOrientation, f, i);
// LinkScriptFunction(MotionComponent, getYaw, f, );
// LinkScriptFunction(MotionComponent, getPitch, f, );
// LinkScriptFunction(MotionComponent, getRoll, f, );
// LinkScriptFunction(MotionComponent, getProjectedAngle, f, );
// LinkScriptFunction(MotionComponent, setProjectedAngleOffset, v, ff);
// LinkScriptFunction(MotionComponent, getActivity, f, );
// LinkScriptFunction(MotionComponent, getThrowState, i, );
// LinkScriptFunctionsEnd

// DeclareScriptFunctionReturn1(MotionComponent, getOrientation, float, uint32_t)
// {
//     return arg1 >= 3 ? 0.0f : orientation[arg1];
// }
// DeclareScriptFunctionReturn0(MotionComponent, getYaw, float) { return orientation[0]; }
// DeclareScriptFunctionReturn0(MotionComponent, getPitch, float) { return orientation[1]; }
// DeclareScriptFunctionReturn0(MotionComponent, getRoll, float) { return orientation[2]; }
// DeclareScriptFunctionReturn0(MotionComponent, getProjectedAngle, float) { return projectedAngle; }
// DeclareScriptFunctionVoid2(MotionComponent, setProjectedAngleOffset, float, float) { setProjectAngleOffset(arg1, arg2); }
// DeclareScriptFunctionReturn0(MotionComponent, getActivity, float) { return activity; }
// DeclareScriptFunctionReturn0(MotionComponent, getThrowState, uint32_t) { return throwState; }
#endif

EndDeclareComponent