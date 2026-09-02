#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "D3D12Renderer.h"
#include "TextureLoader.h"
#include "UploadContext.h"
#include "ModelLoader.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include "Animation.h"
#include "Model.h"
#include <algorithm>

void ResourceManager::Initialize(D3D12Renderer* renderer, TextureLoader* textureLoader, UploadContext* uploadContext)
{
    mRenderer = renderer;
    mTextureLoader = textureLoader;
    mUploadContext = uploadContext;

    mAssetDB.Initialize("Assets");
}

std::shared_ptr<Mesh> ResourceManager::GetCubeMesh()
{
    if (mCubeMesh) return mCubeMesh;

    std::vector<Vertex> vertices =
    {
        // Front Face (-Z)
        { { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Back Face (+Z)
        { {  0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Left Face (-X)
        { { -0.5f, -0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Right Face (+X)
        { {  0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Top Face (+Y)
        { { -0.5f,  0.5f, -0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Bottom Face (-Y)
        { { -0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
    };

    std::vector<UINT32> indices =
    {
        0, 1, 2,  0, 2, 3,
        4, 5, 6,  4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23,
    };

    mCubeMesh = std::make_shared<Mesh>();
    mCubeMesh->Create(mRenderer->GetDevice(), mUploadContext, vertices, indices);
    return mCubeMesh;
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::wstring& path)
{
    if (!mTextureLoader) return nullptr;
    return mTextureLoader->LoadTextureFromFile(path);
}

std::shared_ptr<Texture> ResourceManager::GetDefaultWhiteTexture()
{
    if (!mDefaultWhiteTexture && mTextureLoader)
    {
        mDefaultWhiteTexture = mTextureLoader->CreateSolidColorTexture(255, 255, 255, 255);
    }
    return mDefaultWhiteTexture;
}

std::shared_ptr<Model> ResourceManager::GetOrLoadModel(AssetHandle handle)
{
    if (!handle.IsValid()) return nullptr;

    auto cacheIt = mModelCache.find(handle.GetId());
    if (cacheIt != mModelCache.end()) return cacheIt->second;

    const std::string& path = GetPath(handle);
    if (path.empty()) return nullptr;

    // 3D 모델 확장자 유효성 검사
    std::string ext = "";
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        ext = path.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext != ".gltf" && ext != ".glb" && ext != ".fbx" && ext != ".obj")
    {
        return nullptr;
    }

    auto model = std::make_shared<Model>();
    model->SetHandle(handle);

    // 스킨드 메시 우선 시도 후 실패 시 정적 메시 폴백
    std::vector<std::shared_ptr<Animation>> animations;
    auto skinnedMesh = ModelLoader::LoadSkinnedMesh(mRenderer->GetDevice(), mUploadContext, path, &animations);

    if (skinnedMesh)
    {
        model->SetSkinnedMesh(skinnedMesh);
        model->SetAnimations(std::move(animations));
    }
    else
    {
        auto staticMesh = ModelLoader::LoadStaticMesh(mRenderer->GetDevice(), mUploadContext, path);
        if (!staticMesh) return nullptr;
        model->SetStaticMesh(staticMesh);
    }

    mModelCache[handle.GetId()] = model;
    return model;
}

void ResourceManager::RemapAsset(AssetHandle handle, const std::string& newPath)
{
    if (!handle.IsValid()) return;
    mAssetDB.RegisterOrGetHandle(newPath);
    mModelCache.erase(handle.GetId());
}

void ResourceManager::FlushUploads()
{
    if (mUploadContext)
    {
        mUploadContext->Flush();
    }
}

std::shared_ptr<Texture> ResourceManager::GetOrLoadTexture(AssetHandle handle)
{
    if (!handle.IsValid()) return nullptr;

    auto it = mTextureCache.find(handle.GetId());
    if (it != mTextureCache.end()) return it->second;

    const std::string& pathStr = GetPath(handle);
    if (pathStr.empty()) return nullptr;

    std::wstring widePath(pathStr.begin(), pathStr.end());
    auto texture = LoadTexture(widePath);
    if (texture)
    {
        mTextureCache[handle.GetId()] = texture;
    }

    return texture;
}