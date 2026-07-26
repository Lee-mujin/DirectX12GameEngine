#pragma once
#include <DirectXMath.h>
#include "Vector3.h"
#include "Matrix4x4.h"
#include "MathUtil.h"

struct SpotLight
{
    Vector3 Position{ 0.0f, 0.0f, 0.0f };
    Vector3 Direction{ 0.0f, -1.0f, 0.0f };
    Vector3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
    float Range = 10.0f;

    float InnerConeAngle = 15.0f; // 이 각도 안쪽은 감쇠x
    float OuterConeAngle = 25.0f; // 이 각도 밖은 빛x

    bool Enabled = false;

    // Spot Light용 View-Projection 행렬 계산 함수
    Matrix4x4 GetLightViewProj() const
    {
        using namespace DirectX;

        Vector3 normDir = Direction.GetNormalize();
        Vector3 target = Position + normDir;

        // View 행렬
        Matrix4x4 view = Matrix4x4::LookAt(Position, target, Vector3::Up);

        // Spot Light의 화각(FOV)은 OuterConeAngle * 2
        float fovRadian = Math::Deg2Rad(OuterConeAngle * 2.0f);
        float aspect = 1.0f; // ShadowMap 1:1 비율
        float nearZ = 0.1f;
        float farZ = Range;

        // Perspective Projection 행렬
        XMMATRIX xmProj = XMMatrixPerspectiveFovLH(fovRadian, aspect, nearZ, farZ);
        XMMATRIX xmView = XMMatrixSet(
            view._11, view._12, view._13, view._14,
            view._21, view._22, view._23, view._24,
            view._31, view._32, view._33, view._34,
            view._41, view._42, view._43, view._44
        );

        XMMATRIX vp = xmView * xmProj;

        XMFLOAT4X4 resultXM;
        XMStoreFloat4x4(&resultXM, vp);

        Matrix4x4 result;
        memcpy(&result, &resultXM, sizeof(Matrix4x4));
        return result;
    }
};