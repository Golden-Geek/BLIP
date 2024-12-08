#pragma once

DeclareComponentSingleton(Script, "script", )

    Script script;

bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

virtual bool handleCommandInternal(const String &command, var *data, int numData);

EndDeclareComponent