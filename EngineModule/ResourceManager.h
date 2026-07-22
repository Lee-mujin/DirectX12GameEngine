#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "AssetHandle.h"

enum class AssetType
{
    Unknown,
    Texture,
    Model,
    Scene
};

class Mesh;
class Texture;
class D3D12Renderer;
class TextureLoader;
class UploadContext;
class Model;

class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    void Initialize(D3D12Renderer* renderer, TextureLoader* textureLoader, UploadContext* uploadContext);

    std::shared_ptr<Mesh> GetCubeMesh();
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();

    AssetHandle GetOrCreateHandle(const std::string& path);
    const std::string& GetPath(AssetHandle handle) const;

    std::shared_ptr<Texture> GetOrLoadTexture(AssetHandle handle);
    std::shared_ptr<Model> GetOrLoadModel(AssetHandle handle);

    void RemapAsset(AssetHandle handle, const std::string& newPath);

    //AssetDatabase기반으로 내부 전환 예정
    AssetType GetAssetType(AssetHandle handle) const;

    void FlushUploads();

private:
    D3D12Renderer* mRenderer = nullptr;
    TextureLoader* mTextureLoader = nullptr;
    UploadContext* mUploadContext = nullptr;

    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;

    uint32_t mNextHandleId = 1;
    std::unordered_map<std::string, uint32_t> mPathToId;
    std::unordered_map<uint32_t, std::string> mIdToPath;
    std::unordered_map<uint32_t, std::shared_ptr<Model>> mModelCache;
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> mTextureCache;

    static const std::string kEmptyPath;
};