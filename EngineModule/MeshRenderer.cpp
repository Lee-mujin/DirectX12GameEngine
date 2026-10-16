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

void MeshRenderer::SetMaterial(std::shared_ptr<Material> material, size_t slot)
{
    if (slot >= mMaterialInstances.size())
    {
        mMaterialInstances.resize(slot + 1);
    }
    mMaterialInstances[slot] = MaterialInstance(material);
}

void MeshRenderer::SetMaterials(const std::vector<std::shared_ptr<Material>>& materials)
{
    mMaterialInstances.clear();
    mMaterialInstances.reserve(materials.size());
    for (const auto& mat : materials)
    {
        mMaterialInstances.emplace_back(mat);
    }
}

void MeshRenderer::SetMaterialInstance(const MaterialInstance& matInst, size_t slot)
{
    if (slot >= mMaterialInstances.size())
    {
        mMaterialInstances.resize(slot + 1);
    }
    mMaterialInstances[slot] = matInst;
}

MaterialInstance& MeshRenderer::GetMaterialInstance(size_t slot)
{
    if (slot < mMaterialInstances.size())
    {
        return mMaterialInstances[slot];
    }
    if (mMaterialInstances.empty())
    {
        mMaterialInstances.emplace_back();
        return mMaterialInstances[0];
    }
    return mMaterialInstances[0];
}

const MaterialInstance& MeshRenderer::GetMaterialInstance(size_t slot) const
{
    static const MaterialInstance sDefaultInstance;
    if (slot < mMaterialInstances.size())
    {
        return mMaterialInstances[slot];
    }
    if (!mMaterialInstances.empty())
    {
        return mMaterialInstances[0];
    }
    return sDefaultInstance;
}

void MeshRenderer::SubmitRenderItem(RenderQueue& renderQueue)
{
    if (!mMesh) return;

    const auto& subMeshes = mMesh->GetSubMeshes();
    if (subMeshes.empty()) return;

    Matrix4x4 worldMatrix = GetOwner()->GetTransform().GetMatrix();
    const std::vector<Matrix4x4>* boneMatrices = nullptr;

    if (mSkinnedMesh)
    {
        if (AnimatorComponent* animator = GetOwner()->GetComponent<AnimatorComponent>())
        {
            boneMatrices = &animator->GetFinalBoneMatrices();
        }
    }

    for (const auto& subMesh : subMeshes)
    {
        RenderItem item;
        item.WorldMatrix = worldMatrix;
        item.IndexStart = subMesh.IndexStart;
        item.IndexCount = subMesh.IndexCount;
        item.BaseVertexLocation = subMesh.BaseVertexLocation;

        const MaterialInstance& matInst = GetMaterialInstance(subMesh.MaterialIndex);

        item.BaseColor = matInst.GetBaseColor();
        item.Metallic = matInst.GetMetallic();
        item.Roughness = matInst.GetRoughness();
        item.Shininess = matInst.GetShininess();

        //Base Texture GPU 핸들 추출 (MaterialInstance 오버라이드 지원)
        if (auto baseTex = matInst.GetTexture())
        {
            item.BaseTextureHandle = baseTex->GetSrvHandle();
        }

        //Normal Map GPU 핸들 추출
        if (auto normTex = matInst.GetNormalTexture())
        {
            item.NormalTextureHandle = normTex->GetSrvHandle();
            item.HasNormalMap = true;
        }

        //Skinned Mesh 및 Bone Matrix 세팅
        if (mSkinnedMesh)
        {
            item.SkinnedMeshPtr = mSkinnedMesh.get();
            item.BoneMatrices = boneMatrices;
        }
        else
        {
            item.MeshPtr = mMesh.get();
        }

        renderQueue.Add(item);
    }
}