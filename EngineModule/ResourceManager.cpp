#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "D3D12Renderer.h"
#include "TextureLoader.h"
#include "ModelLoader.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include "Animation.h"
#include "Model.h"

const std::string ResourceManager::kEmptyPath = "";

//TextureLoader 포인터도 함께 주입받아 저장
void ResourceManager::Initialize(D3D12Renderer* renderer, TextureLoader* textureLoader)
{
    mRenderer = renderer;
    mTextureLoader = textureLoader;
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
    //주입받은 텍스처로더로부터 에셋을 불러옴
    return mTextureLoader->LoadTextureFromFile(path);
}

std::shared_ptr<Texture> ResourceManager::GetDefaultWhiteTexture()
{
    if (!mDefaultWhiteTexture)
    {
        //텍스처를 생성
        mDefaultWhiteTexture = mTextureLoader->CreateSolidColorTexture(255, 255, 255, 255);
    }
    return mDefaultWhiteTexture;
}

AssetHandle ResourceManager::GetOrCreateHandle(const std::string& path)
{
    auto it = mPathToId.find(path);
    if (it != mPathToId.end())
    {
        return AssetHandle(it->second);
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

void ResourceManager::RemapAsset(AssetHandle handle, const std::string& newPath)
{
    if (!handle.IsValid())
    {
        return;
    }

    auto oldPathIt = mIdToPath.find(handle.GetId());
    if (oldPathIt != mIdToPath.end())
    {
        mPathToId.erase(oldPathIt->second);
    }

    mIdToPath[handle.GetId()] = newPath;
    mPathToId[newPath] = handle.GetId();

    mModelCache.erase(handle.GetId());
}