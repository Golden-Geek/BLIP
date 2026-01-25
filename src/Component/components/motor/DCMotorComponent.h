#pragma once

#ifndef DC_MOTOR_DEFAULT_EN_PIN
#define DC_MOTOR_DEFAULT_EN_PIN -1
#endif

#ifndef DC_MOTOR_DEFAULT_DIR1_PIN
#define DC_MOTOR_DEFAULT_DIR1_PIN -1
#endif

#ifndef DC_MOTOR_DEFAULT_DIR2_PIN
#define DC_MOTOR_DEFAULT_DIR2_PIN -1
#endif

DeclareComponent(DCMotor, "DCMotor", )

    int curPin = -1;
    bool ledCAttached = false;


    EndDeclareComponent