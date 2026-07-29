#include "pch.h"
#include "BoundingSphere.h"
#include "MathUtil.h"
#include <cmath>

BoundingSphere BoundingSphere::Transform(const Matrix4x4& worldMatrix) const
{
    //Center 위치 변환
    Vector3 newCenter(
        worldMatrix._41 + Center.X * worldMatrix._11 + Center.Y * worldMatrix._21 + Center.Z * worldMatrix._31,
        worldMatrix._42 + Center.X * worldMatrix._12 + Center.Y * worldMatrix._22 + Center.Z * worldMatrix._32,
        worldMatrix._43 + Center.X * worldMatrix._13 + Center.Y * worldMatrix._23 + Center.Z * worldMatrix._33
    );

    //Scale 축 중 최대값 추출하여 반지름 스케일링
    float scaleX = Vector3(worldMatrix._11, worldMatrix._12, worldMatrix._13).GetSize();
    float scaleY = Vector3(worldMatrix._21, worldMatrix._22, worldMatrix._23).GetSize();
    float scaleZ = Vector3(worldMatrix._31, worldMatrix._32, worldMatrix._33).GetSize();
    float maxScale = Math::Max3(scaleX, scaleY, scaleZ);

    return BoundingSphere(newCenter, Radius * maxScale);
}

bool BoundingSphere::Intersects(const BoundingSphere& other) const
{
    float distSq = (Center - other.Center).GetSizeSq();
    float radiusSum = Radius + other.Radius;
    return distSq <= (radiusSum * radiusSum);
}

bool BoundingSphere::Intersects(const Ray& ray, float& outDistance) const
{
    Vector3 oc = ray.Origin - Center;
    float b = Vector3::Dot(oc, ray.Direction);
    float c = Vector3::Dot(oc, oc) - (Radius * Radius);

    //Ray 원점이 구 외부에 있고, 구 반대쪽을 바라보는 경우
    if (c > 0.0f && b > 0.0f) return false;

    float discriminant = b * b - c;
    if (discriminant < 0.0f) return false; //허근 (안 만남)

    float t = -b - std::sqrtf(discriminant);
    if (t < 0.0f) t = 0.0f; //Ray 원점이 구 내부에 있을 때

    outDistance = t;
    return true;
}