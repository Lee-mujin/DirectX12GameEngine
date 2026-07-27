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

    //Material 설정 시 내부적으로 MaterialInstance 자동 바인딩
    void SetMaterial(std::shared_ptr<Material> material);
    void SetMaterialInstance(const MaterialInstance& matInst) { mMaterialInstance = matInst; }

    MaterialInstance& GetMaterialInstance() { return mMaterialInstance; }
    const MaterialInstance& GetMaterialInstance() const { return mMaterialInstance; }

    void SubmitRenderItem(RenderQueue& renderQueue);

private:
    std::shared_ptr<Mesh> mMesh;
    MaterialInstance mMaterialInstance; //개별 오버라이드가 가능한 MaterialInstance 보유

    std::shared_ptr<SkinnedMesh> mSkinnedMesh;
};