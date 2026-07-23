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

const std::string ResourceManager::kEmptyPath = "";

void ResourceManager::Initialize(D3D12Renderer* renderer, TextureLoader* textureLoader, UploadContext* uploadContext)
{
    mRenderer = renderer;
    mTextureLoader = textureLoader;
    mUploadContext = uploadContext;
}

std::shared_ptr<Mesh> ResourceManager::GetCubeMesh()
{
    if (mCubeMesh) return mCubeMesh;

    // Vertex 순서: { position, normal, tangent, color, uv }
    std::vector<Vertex> vertices =
    {
        // Front Face (-Z) : Normal {0, 0, -1}, Tangent {1, 0, 0, 1}
        { { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Back Face (+Z) : Normal {0, 0, 1}, Tangent {-1, 0, 0, 1}
        { {  0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Left Face (-X) : Normal {-1, 0, 0}, Tangent {0, 0, 1, 1}
        { { -0.5f, -0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Right Face (+X) : Normal {1, 0, 0}, Tangent {0, 0, -1, 1}
        { {  0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Top Face (+Y) : Normal {0, 1, 0}, Tangent {1, 0, 0, 1}
        { { -0.5f,  0.5f, -0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },

        // Bottom Face (-Y) : Normal {0, -1, 0}, Tangent {1, 0, 0, 1}
        { { -0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
    };

    std::vector<UINT16> indices =
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
    return mTextureLoader->LoadTextureFromFile(path);
}

std::shared_ptr<Texture> ResourceManager::GetDefaultWhiteTexture()
{
    if (!mDefaultWhiteTexture)
    {
        mDefaultWhiteTexture = mTextureLoader->CreateSolidColorTexture(255, 255, 255, 255);
    }
    return mDefaultWhiteTexture;
}

AssetHandle ResourceManager::GetOrCreateHandle(const std::string& path)
{
    auto it = mPathToId.find(path);
    if (it != mPathToId.end()) return AssetHandle(it->second);

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
    if (!handle.IsValid()) return nullptr;

    auto cacheIt = mModelCache.find(handle.GetId());
    if (cacheIt != mModelCache.end()) return cacheIt->second;

    const std::string& path = GetPath(handle);
    if (path.empty()) return nullptr;

    auto model = std::make_shared<Model>();
    model->SetHandle(handle);

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

    auto oldPathIt = mIdToPath.find(handle.GetId());
    if (oldPathIt != mIdToPath.end())
    {
        mPathToId.erase(oldPathIt->second);
    }

    mIdToPath[handle.GetId()] = newPath;
    mPathToId[newPath] = handle.GetId();
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
    if (it != mTextureCache.end())
    {
        return it->second;
    }

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

//현재는 확장자 기반이나 추후 Metadata 기반(return metadata.Type)으로 교체
AssetType ResourceManager::GetAssetType(AssetHandle handle) const
{
    if (!handle.IsValid()) return AssetType::Unknown;

    const std::string& pathStr = GetPath(handle);
    if (pathStr.empty()) return AssetType::Unknown;

    std::filesystem::path path(pathStr);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".dds" || ext == ".tga")
    {
        return AssetType::Texture;
    }
    else if (ext == ".gltf" || ext == ".glb" || ext == ".fbx" || ext == ".obj")
    {
        return AssetType::Model;
    }
    else if (ext == ".txt" || ext == ".json" || ext == ".scene")
    {
        return AssetType::Scene;
    }

    return AssetType::Unknown;
}