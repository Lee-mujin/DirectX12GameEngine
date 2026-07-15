#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class Mesh;
class SkinnedMesh;
class Texture;
class D3D12Renderer;
class Skeleton;
class Animation;

struct CachedSkinnedModel
{
    std::shared_ptr<SkinnedMesh> Mesh;
    std::vector<std::shared_ptr<Animation>> Animations;
};

class ResourceManager
{
public:
    void Initialize(D3D12Renderer* renderer);
    std::shared_ptr<Mesh> GetCubeMesh();
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();

    std::shared_ptr<Mesh> GetOrLoadStaticModel(const std::string& path);
    const CachedSkinnedModel* GetOrLoadSkinnedModel(const std::string& path);

private:
    D3D12Renderer* mRenderer = nullptr;
    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;

    std::unordered_map<std::string, std::shared_ptr<Mesh>> mStaticModelCache;
    std::unordered_map<std::string, CachedSkinnedModel> mSkinnedModelCache;
};