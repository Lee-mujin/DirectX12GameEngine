#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "AssetDatabase.h"
#include "Texture.h"
#include "Mesh.h"
#include "Model.h"

class D3D12Renderer;
class TextureLoader;
class UploadContext;

//AssetDatabase와 연동하여 리소스 메모리 로딩/캐싱 전담
class ResourceManager
{
public:
    ResourceManager() = default;

    void Initialize(D3D12Renderer* renderer, TextureLoader* textureLoader, UploadContext* uploadContext);

    // AssetDatabase 래퍼 메소드
    AssetDatabase& GetAssetDatabase() { return mAssetDB; }
    AssetHandle GetOrCreateHandle(const std::string& path) { return mAssetDB.RegisterOrGetHandle(path); }
    const std::string& GetPath(AssetHandle handle) const { return mAssetDB.GetPath(handle); }
    AssetType GetAssetType(AssetHandle handle) const { return mAssetDB.GetAssetType(handle); }
    void RemapAsset(AssetHandle handle, const std::string& newPath);

    // GPU 리소스 생성 및 캐싱 로더
    std::shared_ptr<Mesh> GetCubeMesh(); // const 제거로 cpp와 호환
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();
    std::shared_ptr<Texture> GetOrLoadTexture(AssetHandle handle);
    std::shared_ptr<Model> GetOrLoadModel(AssetHandle handle);

    void FlushUploads();

private:
    AssetDatabase mAssetDB; //에셋 핸들 및 경로 매핑 전담 DB

    D3D12Renderer* mRenderer = nullptr;
    TextureLoader* mTextureLoader = nullptr;
    UploadContext* mUploadContext = nullptr;

    std::unordered_map<uint64_t, std::shared_ptr<Texture>> mTextureCache;
    std::unordered_map<uint64_t, std::shared_ptr<Model>> mModelCache;

    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;
};