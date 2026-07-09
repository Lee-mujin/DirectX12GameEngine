#include "pch.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "D3D12Renderer.h"
#include "SkinnedMesh.h"

void MeshRenderer::Render(D3D12Renderer& renderer)
{
    if (!mMesh || !mMaterial) return;

    if (auto skinnedMesh = std::dynamic_pointer_cast<SkinnedMesh>(mMesh)) //SkinnedMesh의 Mesh를 dynamic_cast로 구분
    {
        renderer.DrawSkinnedMesh(*skinnedMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix());
    }
    else
    {
        renderer.DrawMesh(*mMesh, *mMaterial, GetOwner()->GetTransform().GetMatrix());
    }
}