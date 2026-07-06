#pragma once
#include "Vector3.h"

struct PointLight
{
    Vector3 Position{ 0.0f, 0.0f, 0.0f };
    Vector3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;

    float Range = 10.0f; //이 거리 밖에서는 빛 감쇠

    bool Enabled = false; //on/off
};