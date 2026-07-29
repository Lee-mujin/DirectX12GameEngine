#include "pch.h"
#include "BoundingBox.h"
#include "MathUtil.h"
#include <cmath>
#include <algorithm>
#include <limits>

void BoundingBox::Encapsulate(const Vector3& point)
{
    Min = Vector3::Min(Min, point);
    Max = Vector3::Max(Max, point);
}

BoundingBox BoundingBox::Transform(const Matrix4x4& worldMatrix) const
{
    if (!IsValid()) return BoundingBox();

    //Center, Extents 기반 Fast AABB Transformation (Jim Arvo)
    Vector3 center = GetCenter();
    Vector3 extents = GetExtents();

    //Translation 적용된 새로운 Center
    Vector3 newCenter(
        worldMatrix._41 + center.X * worldMatrix._11 + center.Y * worldMatrix._21 + center.Z * worldMatrix._31,
        worldMatrix._42 + center.X * worldMatrix._12 + center.Y * worldMatrix._22 + center.Z * worldMatrix._32,
        worldMatrix._43 + center.X * worldMatrix._13 + center.Y * worldMatrix._23 + center.Z * worldMatrix._33
    );

    //회전/스케일 행렬의 절대값을 적용한 새로운 Extents
    Vector3 newExtents(
        std::abs(worldMatrix._11) * extents.X + std::abs(worldMatrix._21) * extents.Y + std::abs(worldMatrix._31) * extents.Z,
        std::abs(worldMatrix._12) * extents.X + std::abs(worldMatrix._22) * extents.Y + std::abs(worldMatrix._32) * extents.Z,
        std::abs(worldMatrix._13) * extents.X + std::abs(worldMatrix._23) * extents.Y + std::abs(worldMatrix._33) * extents.Z
    );

    return BoundingBox(newCenter - newExtents, newCenter + newExtents);
}

bool BoundingBox::Intersects(const BoundingBox& other) const
{
    if (Max.X < other.Min.X || Min.X > other.Max.X) return false;
    if (Max.Y < other.Min.Y || Min.Y > other.Max.Y) return false;
    if (Max.Z < other.Min.Z || Min.Z > other.Max.Z) return false;
    return true;
}

//Slab Method (슬랩 기법) 기반 Ray vs AABB 교차 판정
bool BoundingBox::Intersects(const Ray& ray, float& outDistance) const
{
    float tMin = 0.0f;
    float tMax = (std::numeric_limits<float>::max)();

    //X축 검사
    if (std::abs(ray.Direction.X) < Math::SMALL_NUMBER)
    {
        if (ray.Origin.X < Min.X || ray.Origin.X > Max.X) return false;
    }
    else
    {
        float invD = 1.0f / ray.Direction.X;
        float t1 = (Min.X - ray.Origin.X) * invD;
        float t2 = (Max.X - ray.Origin.X) * invD;
        if (t1 > t2) std::swap(t1, t2);

        tMin = (std::max)(tMin, t1);
        tMax = (std::min)(tMax, t2);
        if (tMin > tMax) return false;
    }

    //Y축 검사
    if (std::abs(ray.Direction.Y) < Math::SMALL_NUMBER)
    {
        if (ray.Origin.Y < Min.Y || ray.Origin.Y > Max.Y) return false;
    }
    else
    {
        float invD = 1.0f / ray.Direction.Y;
        float t1 = (Min.Y - ray.Origin.Y) * invD;
        float t2 = (Max.Y - ray.Origin.Y) * invD;
        if (t1 > t2) std::swap(t1, t2);

        tMin = (std::max)(tMin, t1);
        tMax = (std::min)(tMax, t2);
        if (tMin > tMax) return false;
    }

    //Z축 검사
    if (std::abs(ray.Direction.Z) < Math::SMALL_NUMBER)
    {
        if (ray.Origin.Z < Min.Z || ray.Origin.Z > Max.Z) return false;
    }
    else
    {
        float invD = 1.0f / ray.Direction.Z;
        float t1 = (Min.Z - ray.Origin.Z) * invD;
        float t2 = (Max.Z - ray.Origin.Z) * invD;
        if (t1 > t2) std::swap(t1, t2);

        tMin = (std::max)(tMin, t1);
        tMax = (std::min)(tMax, t2);
        if (tMin > tMax) return false;
    }

    outDistance = tMin;
    return true;
}