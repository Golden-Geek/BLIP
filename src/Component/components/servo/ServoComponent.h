#pragma once

#ifndef SERVO_DEFAULT_PIN
#define SERVO_DEFAULT_PIN 13
#endif

DeclareComponent(Servo, "servo", )

DeclareIntParam(pin, SERVO_DEFAULT_PIN);
DeclareFloatParam(position, 0);

Servo servo;

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;
void paramValueChangedInternal(void *param) override;

EndDeclareComponent