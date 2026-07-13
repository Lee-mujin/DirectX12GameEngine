#include "pch.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "D3D12Renderer.h"
#include "SkinnedMesh.h"
#include "AnimatorComponent.h"

void MeshRenderer::Render(D3D12Renderer& renderer)
{
    if (!mMesh || !mMaterial) return;

    if (auto skinnedMesh = std::dynamic_pointer_cast<SkinnedMesh>(mMesh)) //SkinnedMesh의 Mesh를 dynamic_cast로 구분
    {
        AnimatorComponent* animator = GetOwner()->GetComponent<AnimatorComponent>();

        static const std::vector<Matrix4x4> kIdentityFallback; //animator 없으면 빈 벡터 -> 렌더러가 항등행렬로 채움

        const std::vector<Matrix4x4>& boneMatrices = animator ? animator->GetFinalBoneMatrices() : kIdentityFallback;

        renderer.DrawSkinnedMesh(*skinnedMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix(), boneMatrices);
    }
    else
    {
        renderer.DrawMesh(*mMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix());
    }
}