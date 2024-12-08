#pragma once
DeclareComponentSingleton(Sequence, "sequence",)

void setupInternal(JsonObject o) override;
void updateInternal() override;
void clearInternal() override;

EndDeclareComponent