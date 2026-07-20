#include "pch.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "D3D12Renderer.h"
#include "SkinnedMesh.h"
#include "AnimatorComponent.h"

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> mesh)
{
    mMesh = mesh;

    //메쉬가 세팅되는 최초 시점에 한번만 타입 체크
    mSkinnedMesh = std::dynamic_pointer_cast<SkinnedMesh>(mesh);
}

void MeshRenderer::Render(D3D12Renderer& renderer)
{
    if (!mMesh || !mMaterial) return;

    //dynamic_cast 없이 널 체크만으로 빠르게 판별합니다.
    if (mSkinnedMesh)
    {
        AnimatorComponent* animator = GetOwner()->GetComponent<AnimatorComponent>();

        static const std::vector<Matrix4x4> kIdentityFallback;
        const std::vector<Matrix4x4>& boneMatrices = animator ? animator->GetFinalBoneMatrices() : kIdentityFallback;

        //mSkinnedMesh 포인터를 그대로 전달
        renderer.DrawSkinnedMesh(*mSkinnedMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix(), boneMatrices);
    }
    else
    {
        renderer.DrawMesh(*mMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix());
    }
}