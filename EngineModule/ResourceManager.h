// ResourceManager.h
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "AssetHandle.h"

class Mesh;
class Texture;
class D3D12Renderer;
class TextureLoader;
class Model;

class ResourceManager
{
public:
    //초기화 시 렌더러와 함께 텍스처 로더를 인자로 받도록 변경
    void Initialize(D3D12Renderer* renderer, TextureLoader* textureLoader);

    std::shared_ptr<Mesh> GetCubeMesh();
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();

    //경로 -> 핸들 발급, 이미 있으면 기존 핸들 재사용
    AssetHandle GetOrCreateHandle(const std::string& path);

    //핸들 -> 실제 경로 조회,재매핑되면 여기서 새 경로
    const std::string& GetPath(AssetHandle handle) const;

    //핸들 -> Model 조회/로드
    std::shared_ptr<Model> GetOrLoadModel(AssetHandle handle);

    //같은 핸들이 이제 새 경로를 가리키게 함
    void RemapAsset(AssetHandle handle, const std::string& newPath);

private:
    D3D12Renderer* mRenderer = nullptr;
    TextureLoader* mTextureLoader = nullptr; //주입받은 텍스처 로더를 보관할 멤버 변수

    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;

    uint32_t mNextHandleId = 1;
    std::unordered_map<std::string, uint32_t> mPathToId;   // 경로 -> id (중복 발급 방지)
    std::unordered_map<uint32_t, std::string> mIdToPath;   // id -> 현재 경로 (재매핑되면 갱신)
    std::unordered_map<uint32_t, std::shared_ptr<Model>> mModelCache; // id -> 로드된 Model

    static const std::string kEmptyPath;
};