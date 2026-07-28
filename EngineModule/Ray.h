#pragma once
#include "Vector3.h"

struct Ray
{
    Vector3 Origin = Vector3::Zero;
    Vector3 Direction = Vector3::Forward;

    Ray() = default;
    Ray(const Vector3& origin, const Vector3& direction)
        : Origin(origin), Direction(direction.GetNormalize()) {}

    // Ray 상의 특정 거리 t 지점 좌표 반환 (P = Origin + Direction * t)
    Vector3 GetPoint(float distance) const
    {
        return Origin + (Direction * distance);
    }
};