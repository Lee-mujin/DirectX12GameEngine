#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Ray.h"
#include <limits>

struct BoundingBox
{
    Vector3 Min{ (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
    Vector3 Max{ -(std::numeric_limits<float>::max)(), -(std::numeric_limits<float>::max)(), -(std::numeric_limits<float>::max)() };

    BoundingBox() = default;
    BoundingBox(const Vector3& min, const Vector3& max) : Min(min), Max(max) {}

    // 박스 유효성 확인
    bool IsValid() const
    {
        return Min.X <= Max.X && Min.Y <= Max.Y && Min.Z <= Max.Z;
    }

    // 단일 정점 포함 확대
    void Encapsulate(const Vector3& point);

    // 중심점 및 크기 반환
    Vector3 GetCenter() const { return (Min + Max) * 0.5f; }
    Vector3 GetExtents() const { return (Max - Min) * 0.5f; }
    Vector3 GetSize() const { return Max - Min; }

    // World Matrix 적용 변환 (Jim Arvo의 Fast Transform)
    BoundingBox Transform(const Matrix4x4& worldMatrix) const;

    // 교차 연산
    bool Intersects(const BoundingBox& other) const;
    bool Intersects(const Ray& ray, float& outDistance) const;
};