#include "pch.h"
#include "RenderSystem.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Camera.h"
#include "BoundingBox.h"

namespace
{
    struct Plane
    {
        float A = 0.0f, B = 0.0f, C = 0.0f, D = 0.0f;
    };

    struct Frustum
    {
        Plane Planes[6];
    };

    // ViewProj(행 우선, row-vector 규약)에서 6개의 클립平面을 추출한다.
    // DirectX LH 투영에서 clip = v * (View * Proj) 이므로
    //   left: cx + cw >= 0, right: -cx + cw >= 0, ...
    Frustum ExtractFrustum(const Matrix4x4& vp)
    {
        Frustum f;
        const float* m = reinterpret_cast<const float*>(&vp);

        // _ij = m[(i-1)*4 + (j-1)]
        f.Planes[0] = { m[0] + m[3],  m[4] + m[7],  m[8] + m[11],  m[12] + m[15] }; // Left
        f.Planes[1] = { -m[0] + m[3], -m[4] + m[7], -m[8] + m[11], -m[12] + m[15] }; // Right
        f.Planes[2] = { m[1] + m[3],  m[5] + m[7],  m[9] + m[11],  m[13] + m[15] }; // Bottom
        f.Planes[3] = { -m[1] + m[3], -m[5] + m[7], -m[9] + m[11], -m[13] + m[15] }; // Top
        f.Planes[4] = { m[2],         m[6],         m[10],         m[14] };          // Near
        f.Planes[5] = { -m[2] + m[3], -m[6] + m[7], -m[10] + m[11], -m[14] + m[15] }; // Far
        return f;
    }

    bool IsBoxInsideFrustum(const BoundingBox& box, const Frustum& frustum)
    {
        if (!box.IsValid()) return true; // 유효하지 않은 바운드는 보수적으로 유지

        const Vector3 center = box.GetCenter();
        const Vector3 extents = box.GetExtents();

        for (const Plane& p : frustum.Planes)
        {
            const float distance =
                p.A * center.X + p.B * center.Y + p.C * center.Z + p.D;

            // AABB의 평면상 투영 반경 (여유 2%를 더해 경계에서 깜빡임 방지)
            const float radius =
                (fabsf(p.A) * extents.X + fabsf(p.B) * extents.Y + fabsf(p.C) * extents.Z) * 1.02f;

            if (distance + radius < 0.0f)
            {
                return false;
            }
        }
        return true;
    }
}

void RenderSystem::BuildRenderQueue(const std::vector<std::shared_ptr<GameObject>>& gameObjects,
    RenderQueue& outQueue, const Camera* camera, UINT* outCulledCount)
{
    outQueue.Clear();
    UINT culled = 0;

    Frustum frustum;
    if (camera)
    {
        frustum = ExtractFrustum(camera->GetView() * camera->GetProjection());
    }

    for (const auto& obj : gameObjects)
    {
        if (!obj) continue;

        if (MeshRenderer* meshRenderer = obj->GetComponent<MeshRenderer>())
        {
            // 월드 AABB로保守적 컬링. 스키닝 메시의 경우 bind-pose 기준이므로
            // 애니메이션으로 크게 움직이는 오브젝트는 여유를 두기 위해 1.25배 확장한다.
            if (camera)
            {
                auto mesh = meshRenderer->GetMesh();
                if (mesh && mesh->GetBoundingBox().IsValid())
                {
                    BoundingBox worldBox = mesh->GetBoundingBox().Transform(obj->GetTransform().GetMatrix());
                    if (meshRenderer->GetSkinnedMesh())
                    {
                        const Vector3 c = worldBox.GetCenter();
                        const Vector3 e = worldBox.GetExtents() * 1.25f;
                        worldBox = BoundingBox(c - e, c + e);
                    }
                    if (!IsBoxInsideFrustum(worldBox, frustum))
                    {
                        ++culled;
                        continue;
                    }
                }
            }

            meshRenderer->SubmitRenderItem(outQueue);
        }
    }

    if (outCulledCount)
    {
        *outCulledCount = culled;
    }
}
