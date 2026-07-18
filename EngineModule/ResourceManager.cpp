#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "D3D12Renderer.h"
#include "ModelLoader.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include "Animation.h"
#include "Model.h"

const std::string ResourceManager::kEmptyPath = "";

void ResourceManager::Initialize(D3D12Renderer* renderer)
{
    mRenderer = renderer;
}

std::shared_ptr<Mesh> ResourceManager::GetCubeMesh()
{
    if (mCubeMesh)
    {
        return mCubeMesh;
    }

    std::vector<Vertex> vertices =
    {
        // Front -Z, normal (0,0,-1)
        { { -0.5f, -0.5f, -0.5f }, { 0,0,-1 }, {1,1,1,1}, {0.0f,1.0f} },
        { { -0.5f,  0.5f, -0.5f }, { 0,0,-1 }, {1,1,1,1}, {0.0f,0.0f} },
        { {  0.5f,  0.5f, -0.5f }, { 0,0,-1 }, {1,1,1,1}, {1.0f,0.0f} },
        { {  0.5f, -0.5f, -0.5f }, { 0,0,-1 }, {1,1,1,1}, {1.0f,1.0f} },

        // Back +Z, normal (0,0,1)
        { {  0.5f, -0.5f,  0.5f }, { 0,0,1 }, {1,1,1,1}, {0.0f,1.0f} },
        { {  0.5f,  0.5f,  0.5f }, { 0,0,1 }, {1,1,1,1}, {0.0f,0.0f} },
        { { -0.5f,  0.5f,  0.5f }, { 0,0,1 }, {1,1,1,1}, {1.0f,0.0f} },
        { { -0.5f, -0.5f,  0.5f }, { 0,0,1 }, {1,1,1,1}, {1.0f,1.0f} },

        // Left -X, normal (-1,0,0)
        { { -0.5f, -0.5f,  0.5f }, { -1,0,0 }, {1,1,1,1}, {0.0f,1.0f} },
        { { -0.5f,  0.5f,  0.5f }, { -1,0,0 }, {1,1,1,1}, {0.0f,0.0f} },
        { { -0.5f,  0.5f, -0.5f }, { -1,0,0 }, {1,1,1,1}, {1.0f,0.0f} },
        { { -0.5f, -0.5f, -0.5f }, { -1,0,0 }, {1,1,1,1}, {1.0f,1.0f} },

        // Right +X, normal (1,0,0)
        { {  0.5f, -0.5f, -0.5f }, { 1,0,0 }, {1,1,1,1}, {0.0f,1.0f} },
        { {  0.5f,  0.5f, -0.5f }, { 1,0,0 }, {1,1,1,1}, {0.0f,0.0f} },
        { {  0.5f,  0.5f,  0.5f }, { 1,0,0 }, {1,1,1,1}, {1.0f,0.0f} },
        { {  0.5f, -0.5f,  0.5f }, { 1,0,0 }, {1,1,1,1}, {1.0f,1.0f} },

        // Top +Y, normal (0,1,0)
        { { -0.5f,  0.5f, -0.5f }, { 0,1,0 }, {1,1,1,1}, {0.0f,1.0f} },
        { { -0.5f,  0.5f,  0.5f }, { 0,1,0 }, {1,1,1,1}, {0.0f,0.0f} },
        { {  0.5f,  0.5f,  0.5f }, { 0,1,0 }, {1,1,1,1}, {1.0f,0.0f} },
        { {  0.5f,  0.5f, -0.5f }, { 0,1,0 }, {1,1,1,1}, {1.0f,1.0f} },

        // Bottom -Y, normal (0,-1,0)
        { { -0.5f, -0.5f,  0.5f }, { 0,-1,0 }, {1,1,1,1}, {0.0f,1.0f} },
        { { -0.5f, -0.5f, -0.5f }, { 0,-1,0 }, {1,1,1,1}, {0.0f,0.0f} },
        { {  0.5f, -0.5f, -0.5f }, { 0,-1,0 }, {1,1,1,1}, {1.0f,0.0f} },
        { {  0.5f, -0.5f,  0.5f }, { 0,-1,0 }, {1,1,1,1}, {1.0f,1.0f} },
    };

    std::vector<UINT16> indices =
    {
        0, 1, 2,  0, 2, 3, //Front
        4, 5, 6,  4, 6, 7, //Back
        8, 9, 10, 8, 10, 11, //Left
        12,13,14, 12,14,15, //Right
        16,17,18, 16,18,19, //Top
        20,21,22, 20,22,23, //Bottom
    };

    mCubeMesh = std::make_shared<Mesh>();
    mCubeMesh->Create(mRenderer->GetDevice(), vertices, indices);
    return mCubeMesh;
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::wstring& path)
{
    return mRenderer->LoadTextureFromFile(path);
}

std::shared_ptr<Texture> ResourceManager::GetDefaultWhiteTexture()
{
    if (!mDefaultWhiteTexture)
    {
        mDefaultWhiteTexture = mRenderer->CreateSolidColorTexture(255, 255, 255, 255);
    }
    return mDefaultWhiteTexture;
}
//문자열 경로는 ContentBrowser와 ResourceManager::GetOrCreateHandle/GetPath 두 지점에서만
//GameObject, Model, Scene, 저장 파일의 AssetPath 필드는 여전히 텍스트지만 로드 시 즉시 핸들로 변환
AssetHandle ResourceManager::GetOrCreateHandle(const std::string& path)
{
    auto it = mPathToId.find(path);
    if (it != mPathToId.end())
    {
        return AssetHandle(it->second); // friend라 private 생성자 접근 가능
    }

    uint32_t id = mNextHandleId++;
    mPathToId[path] = id;
    mIdToPath[id] = path;

    return AssetHandle(id);
}

const std::string& ResourceManager::GetPath(AssetHandle handle) const
{
    auto it = mIdToPath.find(handle.GetId());
    return (it != mIdToPath.end()) ? it->second : kEmptyPath;
}

std::shared_ptr<Model> ResourceManager::GetOrLoadModel(AssetHandle handle)
{
    if (!handle.IsValid())
    {
        return nullptr;
    }

    auto cacheIt = mModelCache.find(handle.GetId());
    if (cacheIt != mModelCache.end())
    {
        return cacheIt->second;
    }

    const std::string& path = GetPath(handle);
    if (path.empty())
    {
        return nullptr;
    }

    auto model = std::make_shared<Model>();
    model->SetHandle(handle);

    std::vector<std::shared_ptr<Animation>> animations;
    auto skinnedMesh = ModelLoader::LoadSkinnedMesh(mRenderer->GetDevice(), path, &animations);

    if (skinnedMesh)
    {
        model->SetSkinnedMesh(skinnedMesh);
        model->SetAnimations(std::move(animations));
    }
    else
    {
        auto staticMesh = ModelLoader::LoadStaticMesh(mRenderer->GetDevice(), path);
        if (!staticMesh)
        {
            return nullptr;
        }
        model->SetStaticMesh(staticMesh);
    }

    mModelCache[handle.GetId()] = model;
    return model;
}
//나중에 파일 rename 감지(Content Browser에서 F2로 이름 바꾸기 등)를 만들면 ResourceManager::RemapAsset() 한 번만 호출
void ResourceManager::RemapAsset(AssetHandle handle, const std::string& newPath)
{
    if (!handle.IsValid())
    {
        return;
    }

    // 이전 경로 매핑 제거
    auto oldPathIt = mIdToPath.find(handle.GetId());
    if (oldPathIt != mIdToPath.end())
    {
        mPathToId.erase(oldPathIt->second);
    }

    mIdToPath[handle.GetId()] = newPath;
    mPathToId[newPath] = handle.GetId();

    mModelCache.erase(handle.GetId()); // 다음 GetOrLoadModel 호출 시 새 경로로 다시 로드
}