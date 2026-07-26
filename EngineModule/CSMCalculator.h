#pragma once
#include <DirectXMath.h>
#include <array>
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "MathUtil.h"

static constexpr UINT kCascadeCount = 3;

struct CascadeData
{
    std::array<Matrix4x4, kCascadeCount> ViewProjMatrices;
    Vector3 SplitDistances; // x: Near Split, y: Mid Split, z: Far Split
};

//Directional Light 및 Shadow Pass에서 사용되는 CSM 수학 연산 전용 클래스
class CSMCalculator
{
public:
    static Matrix4x4 GetLightView(const DirectionalLight& light, const Vector3& sceneCenter)
    {
        Vector3 lightPos = sceneCenter - light.Direction.GetNormalize() * 20.0f;
        return Matrix4x4::LookAt(lightPos, sceneCenter, Vector3::Up);
    }

    static Matrix4x4 GetLightProjection(float orthoSize = 20.0f, float nearZ = 0.1f, float farZ = 50.0f)
    {
        return Matrix4x4::Ortho(orthoSize, orthoSize, nearZ, farZ);
    }

    static Matrix4x4 GetStableLightViewProj(const DirectionalLight& light, const Vector3& sceneCenter = Vector3::Zero, float orthoSize = 20.0f, float nearZ = 0.1f, float farZ = 50.0f, UINT shadowMapSize = 2048)
    {
        using namespace DirectX;

        Matrix4x4 view = GetLightView(light, sceneCenter);
        Matrix4x4 proj = GetLightProjection(orthoSize, nearZ, farZ);

        XMMATRIX xmView = XMMatrixSet(
            view._11, view._12, view._13, view._14,
            view._21, view._22, view._23, view._24,
            view._31, view._32, view._33, view._34,
            view._41, view._42, view._43, view._44
        );

        XMMATRIX xmProj = XMMatrixSet(
            proj._11, proj._12, proj._13, proj._14,
            proj._21, proj._22, proj._23, proj._24,
            proj._31, proj._32, proj._33, proj._34,
            proj._41, proj._42, proj._43, proj._44
        );

        XMMATRIX vp = xmView * xmProj;

        XMVECTOR shadowOrigin = XMVector3TransformCoord(XMVectorZero(), vp);
        shadowOrigin = XMVectorScale(shadowOrigin, static_cast<float>(shadowMapSize) / 2.0f);

        XMVECTOR roundedOrigin = XMVectorFloor(shadowOrigin);
        XMVECTOR roundOffset = XMVectorSubtract(shadowOrigin, roundedOrigin);
        roundOffset = XMVectorScale(roundOffset, 2.0f / static_cast<float>(shadowMapSize));

        XMFLOAT4 offsetFloat;
        XMStoreFloat4(&offsetFloat, roundOffset);

        xmProj.r[3] = XMVectorSet(
            XMVectorGetX(xmProj.r[3]) - offsetFloat.x,
            XMVectorGetY(xmProj.r[3]) - offsetFloat.y,
            XMVectorGetZ(xmProj.r[3]),
            XMVectorGetW(xmProj.r[3])
        );

        XMMATRIX stableVP = xmView * xmProj;

        XMFLOAT4X4 resultXM;
        XMStoreFloat4x4(&resultXM, stableVP);

        Matrix4x4 result;
        memcpy(&result, &resultXM, sizeof(Matrix4x4));
        return result;
    }

    static CascadeData Calculate(const DirectionalLight& light, const Camera& camera, float nearZ = 0.1f, float farZ = 100.0f, UINT shadowMapSize = 2048)
    {
        using namespace DirectX;

        CascadeData data;

        float lambda = 0.5f;
        float clipRange = farZ - nearZ;

        float minZ = nearZ;
        float maxZ = farZ;

        float ratio = maxZ / minZ;
        float cascadeSplits[kCascadeCount];

        float splitRatios[kCascadeCount] = { 0.1f, 0.3f, 1.0f };
        for (UINT i = 0; i < kCascadeCount; ++i)
        {
            float p = splitRatios[i];
            float logSplit = minZ * powf(ratio, p);
            float uniformSplit = minZ + clipRange * p;
            float d = lambda * (logSplit - uniformSplit) + uniformSplit;
            cascadeSplits[i] = d;
        }

        data.SplitDistances = Vector3(cascadeSplits[0], cascadeSplits[1], cascadeSplits[2]);

        Matrix4x4 camView = camera.GetView();
        Vector3 camForward = Vector3(camView._13, camView._23, camView._33).GetNormalize();

        float prevSplitDist = nearZ;
        for (UINT i = 0; i < kCascadeCount; ++i)
        {
            float splitDist = cascadeSplits[i];

            Vector3 camPos = camera.GetPosition();
            Vector3 center = camPos + camForward * ((prevSplitDist + splitDist) * 0.5f);

            float radius = (splitDist - prevSplitDist) * 0.7071f + (splitDist * 0.15f);

            Vector3 lightPos = center - light.Direction.GetNormalize() * (radius * 2.0f);
            Matrix4x4 lightView = Matrix4x4::LookAt(lightPos, center, Vector3::Up);

            Matrix4x4 lightProj = Matrix4x4::Ortho(radius * 2.0f, radius * 2.0f, 0.1f, radius * 4.0f);

            XMMATRIX xmView = XMMatrixSet(
                lightView._11, lightView._12, lightView._13, lightView._14,
                lightView._21, lightView._22, lightView._23, lightView._24,
                lightView._31, lightView._32, lightView._33, lightView._34,
                lightView._41, lightView._42, lightView._43, lightView._44
            );

            XMMATRIX xmProj = XMMatrixSet(
                lightProj._11, lightProj._12, lightProj._13, lightProj._14,
                lightProj._21, lightProj._22, lightProj._23, lightProj._24,
                lightProj._31, lightProj._32, lightProj._33, lightProj._34,
                lightProj._41, lightProj._42, lightProj._43, lightProj._44
            );

            XMMATRIX vp = xmView * xmProj;

            XMVECTOR shadowOrigin = XMVector3TransformCoord(XMVectorZero(), vp);
            shadowOrigin = XMVectorScale(shadowOrigin, static_cast<float>(shadowMapSize) / 2.0f);

            XMVECTOR roundedOrigin = XMVectorFloor(shadowOrigin);
            XMVECTOR roundOffset = XMVectorSubtract(shadowOrigin, roundedOrigin);
            roundOffset = XMVectorScale(roundOffset, 2.0f / static_cast<float>(shadowMapSize));

            XMFLOAT4 offsetFloat;
            XMStoreFloat4(&offsetFloat, roundOffset);

            xmProj.r[3] = XMVectorSet(
                XMVectorGetX(xmProj.r[3]) - offsetFloat.x,
                XMVectorGetY(xmProj.r[3]) - offsetFloat.y,
                XMVectorGetZ(xmProj.r[3]),
                XMVectorGetW(xmProj.r[3])
            );

            XMMATRIX stableVP = xmView * xmProj;
            XMFLOAT4X4 resultXM;
            XMStoreFloat4x4(&resultXM, stableVP);

            Matrix4x4 result;
            memcpy(&result, &resultXM, sizeof(Matrix4x4));
            data.ViewProjMatrices[i] = result;

            prevSplitDist = splitDist;
        }

        return data;
    }
};