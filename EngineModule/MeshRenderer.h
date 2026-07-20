#pragma once
#include <memory>
#include "Component.h"

class Mesh;
class Material;
class SkinnedMesh;

class MeshRenderer : public Component
{
public:
    void SetMesh(std::shared_ptr<Mesh> mesh);
    void SetMaterial(std::shared_ptr<Material> material) { mMaterial = material; }

    void Render(D3D12Renderer& renderer) override;

    std::shared_ptr<Material> GetMaterial() const { return mMaterial; }

private:
    std::shared_ptr<Mesh> mMesh;
    std::shared_ptr<Material> mMaterial;
    
    //캐싱용 멤버 변수 추가
    std::shared_ptr<SkinnedMesh> mSkinnedMesh; 
};