#pragma once
#include <memory>
#include "Component.h"

class Mesh;
class Material;

class MeshRenderer : public Component
{
public:
    void SetMesh(std::shared_ptr<Mesh> mesh) { mMesh = mesh; }
    void SetMaterial(std::shared_ptr<Material> material) { mMaterial = material; }

    void Render(D3D12Renderer& renderer) override;

private:
    std::shared_ptr<Mesh> mMesh;
    std::shared_ptr<Material> mMaterial;
};