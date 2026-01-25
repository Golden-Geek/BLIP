#pragma once

DeclareComponent(Dummy, "dummy", )

DeclareBoolParam(param1, false);
DeclareBoolParam(param2, true);
DeclareIntParam(param3, 1);
DeclareFloatParam(param4, 3.5f);
DeclareStringParam(param5, "cool");


void setupInternal(JsonObject o) override;

EndDeclareComponent



//Manager

DeclareComponentManagerDefaultMax(Dummy, DUMMY, dummies, dummy)
EndDeclareComponent


