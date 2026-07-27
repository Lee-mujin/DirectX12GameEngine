#include "pch.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "RenderQueue.h"
#include "Mesh.h"
#include "SkinnedMesh.h"
#include "Material.h"
#include "Texture.h"
#include "AnimatorComponent.h"

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> mesh)
{
    mMesh = mesh;
    mSkinnedMesh = std::dynamic_pointer_cast<SkinnedMesh>(mesh);
}

void MeshRenderer::SetMaterial(std::shared_ptr<Material> material)
{
    mMaterialInstance = MaterialInstance(material);
}

void MeshRenderer::SubmitRenderItem(RenderQueue& renderQueue)
{
    if (!mMesh) return;

    RenderItem item;
    item.WorldMatrix = GetOwner()->GetTransform().GetMatrix();
    item.BaseColor = mMaterialInstance.GetBaseColor();
    item.Shininess = mMaterialInstance.GetShininess();

    //Base Texture GPU 핸들 추출 (MaterialInstance 오버라이드 지원)
    if (auto baseTex = mMaterialInstance.GetTexture())
    {
        item.BaseTextureHandle = baseTex->GetSrvHandle();
    }

    //Normal Map GPU 핸들 추출
    if (auto normTex = mMaterialInstance.GetNormalTexture())
    {
        item.NormalTextureHandle = normTex->GetSrvHandle();
        item.HasNormalMap = true;
    }

    //Skinned Mesh 및 Bone Matrix 세팅
    if (mSkinnedMesh)
    {
        item.SkinnedMeshPtr = mSkinnedMesh.get();
        if (AnimatorComponent* animator = GetOwner()->GetComponent<AnimatorComponent>())
        {
            item.BoneMatrices = &animator->GetFinalBoneMatrices();
        }
    }
    else
    {
        item.MeshPtr = mMesh.get();
    }

    renderQueue.Add(item);
}