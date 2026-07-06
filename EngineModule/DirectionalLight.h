#pragma once
#include "Vector3.h"

struct DirectionalLight
{
    Vector3 Direction{ 0.0f, -1.0f, 0.0f };
    Vector3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
};