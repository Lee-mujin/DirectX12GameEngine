#pragma once
#include <memory>
#include <vector>
#include "AssetHandle.h"
#include "SkinnedMesh.h"

class Mesh;
class Skeleton;
class Animation;
class Material;

class Model
{
public:
    bool IsSkinned() const { return mSkinnedMesh != nullptr; }

    std::shared_ptr<Mesh> GetMesh() const { return mSkinnedMesh ? mSkinnedMesh : mStaticMesh; }
    std::shared_ptr<SkinnedMesh> GetSkinnedMesh() const { return mSkinnedMesh; }
    std::shared_ptr<Skeleton> GetSkeleton() const;
    const std::vector<std::shared_ptr<Animation>>& GetAnimations() const { return mAnimations; }

    void SetStaticMesh(std::shared_ptr<Mesh> mesh) { mStaticMesh = mesh; }
    void SetSkinnedMesh(std::shared_ptr<SkinnedMesh> mesh) { mSkinnedMesh = mesh; }
    void SetAnimations(std::vector<std::shared_ptr<Animation>> animations) { mAnimations = std::move(animations); }

    //머티리얼(단일 및 멀티 머티리얼 공용) 접근자
    std::shared_ptr<Material> GetMaterial(size_t index = 0) const
    {
        if (index < mMaterials.size()) return mMaterials[index];
        return nullptr;
    }
    const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return mMaterials; }
    void SetMaterials(std::vector<std::shared_ptr<Material>> materials) { mMaterials = std::move(materials); }
    void AddMaterial(std::shared_ptr<Material> material) { mMaterials.push_back(material); }
    void SetMaterial(std::shared_ptr<Material> material, size_t index = 0)
    {
        if (index >= mMaterials.size()) mMaterials.resize(index + 1);
        mMaterials[index] = material;
    }

    AssetHandle GetHandle() const { return mHandle; }
    void SetHandle(AssetHandle handle) { mHandle = handle; }

private:
    std::shared_ptr<Mesh> mStaticMesh;
    std::shared_ptr<SkinnedMesh> mSkinnedMesh;
    std::vector<std::shared_ptr<Animation>> mAnimations;
    std::vector<std::shared_ptr<Material>> mMaterials;
    AssetHandle mHandle;
};