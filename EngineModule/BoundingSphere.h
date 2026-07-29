#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Ray.h"

struct BoundingSphere
{
    Vector3 Center = Vector3::Zero;
    float Radius = 0.0f;

    BoundingSphere() = default;
    BoundingSphere(const Vector3& center, float radius) : Center(center), Radius(radius) {}

    //World Matrix 변환
    BoundingSphere Transform(const Matrix4x4& worldMatrix) const;

    //교차 연산
    bool Intersects(const BoundingSphere& other) const;
    bool Intersects(const Ray& ray, float& outDistance) const;
};