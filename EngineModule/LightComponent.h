#pragma once
#include "Component.h"
#include "Vector3.h"

class LightComponent : public Component
{
public:
    const Vector3& GetColor() const { return mColor; }
    void SetColor(const Vector3& color) { mColor = color; }

    float GetIntensity() const { return mIntensity; }
    void SetIntensity(float intensity) { mIntensity = intensity; }

private:
    Vector3 mColor{ 1.0f, 1.0f, 1.0f };
    float mIntensity = 1.0f;
};