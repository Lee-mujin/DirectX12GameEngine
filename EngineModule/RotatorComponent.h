#pragma once
#include "Component.h"

class RotatorComponent : public Component
{
public:
    explicit RotatorComponent(float degreesPerSecond = 90.0f) : mSpeed(degreesPerSecond) {}
    void Update(float deltaTime) override;

private:
    float mSpeed;
    float mAngle = 0.0f;
};