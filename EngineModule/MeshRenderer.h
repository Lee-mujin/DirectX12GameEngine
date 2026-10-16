#pragma once
#include <memory>
#include "Component.h"
#include "MaterialInstance.h"

class Mesh;
class Material;
class SkinnedMesh;
class RenderQueue;

class MeshRenderer : public Component
{
public:
    void SetMesh(std::shared_ptr<Mesh> mesh);

    //Mesh 및 SkinnedMesh Getter 추가
    std::shared_ptr<Mesh> GetMesh() const { return mMesh; }
    std::shared_ptr<SkinnedMesh> GetSkinnedMesh() const { return mSkinnedMesh; }

    //Material 설정 시 내부적으로 MaterialInstance 자동 바인딩 (단일 및 멀티 머티리얼 슬롯)
    void SetMaterial(std::shared_ptr<Material> material, size_t slot = 0);
    void SetMaterials(const std::vector<std::shared_ptr<Material>>& materials);
    void SetMaterialInstance(const MaterialInstance& matInst, size_t slot = 0);

    MaterialInstance& GetMaterialInstance(size_t slot = 0);
    const MaterialInstance& GetMaterialInstance(size_t slot = 0) const;

    size_t GetMaterialInstanceCount() const { return mMaterialInstances.size(); }
    const std::vector<MaterialInstance>& GetMaterialInstances() const { return mMaterialInstances; }
    std::vector<MaterialInstance>& GetMaterialInstances() { return mMaterialInstances; }

    void SubmitRenderItem(RenderQueue& renderQueue);

private:
    std::shared_ptr<Mesh> mMesh;
    std::vector<MaterialInstance> mMaterialInstances; //개별 오버라이드가 가능한 MaterialInstance 배열

    std::shared_ptr<SkinnedMesh> mSkinnedMesh;
};