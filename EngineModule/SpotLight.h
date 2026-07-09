#pragma once
#include "Vector3.h"

struct SpotLight
{
    Vector3 Position{ 0.0f, 0.0f, 0.0f };
    Vector3 Direction{ 0.0f, -1.0f, 0.0f };
    Vector3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
    float Range = 10.0f;

    float InnerConeAngle = 15.0f; //이 각도 안쪽은 감쇠x
    float OuterConeAngle = 25.0f; //이 각도 밖은 빛x

    bool Enabled = false;
};