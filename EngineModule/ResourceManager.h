#pragma once
#include <memory>
#include <string>

class Mesh;
class SkinnedMesh;
class Texture;
class D3D12Renderer;
class Skeleton;
class Animation;

class ResourceManager
{
public:
    void Initialize(D3D12Renderer* renderer);
    std::shared_ptr<Mesh> GetCubeMesh();
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();
    std::shared_ptr<Mesh> LoadStaticModel(const std::string& path);
    std::shared_ptr<SkinnedMesh> LoadSkinnedModel(const std::string& path, std::vector<std::shared_ptr<Animation>>* outAnimations = nullptr);
private:
    D3D12Renderer* mRenderer = nullptr;
    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;
};