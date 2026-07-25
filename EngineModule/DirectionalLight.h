#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

struct DirectionalLight
{
    Vector3 Direction{ 0.0f, -1.0f, 0.0f };
    Vector3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;

    //Shadow Mapping용 Light Camera 계산
    Matrix4x4 GetLightView(const Vector3& sceneCenter) const
    {
        Vector3 lightPos = sceneCenter - Direction.GetNormalize() * 20.0f; //씬 중심에서 빛 반대 방향으로 멀리
        return Matrix4x4::LookAt(lightPos, sceneCenter, Vector3::Up);
    }

    Matrix4x4 GetLightProjection(float orthoSize = 20.0f, float nearZ = 0.1f, float farZ = 50.0f) const
    {
        return Matrix4x4::Ortho(orthoSize, orthoSize, nearZ, farZ);
    }
};