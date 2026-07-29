#include "pch.h"
#include "AssetDatabase.h"
#include <algorithm>

const std::string AssetDatabase::kEmptyPath = "";

void AssetDatabase::Initialize(const std::string& rootPath)
{
    if (!std::filesystem::exists(rootPath)) return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath))
    {
        if (entry.is_regular_file())
        {
            RegisterOrGetHandle(entry.path().string());
        }
    }
}

AssetHandle AssetDatabase::RegisterOrGetHandle(const std::string& path)
{
    //경로 정규화 (역슬래시 -> 슬래시)
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    auto it = mPathToHandleMap.find(normalizedPath);
    if (it != mPathToHandleMap.end())
    {
        return it->second;
    }

    uint32_t id = mNextHandleId++;
    AssetHandle handle{ id };

    mPathToHandleMap[normalizedPath] = handle;
    mHandleToPathMap[id] = normalizedPath;
    mHandleToTypeMap[id] = DeduceAssetType(normalizedPath);

    return handle;
}

const std::string& AssetDatabase::GetPath(AssetHandle handle) const
{
    auto it = mHandleToPathMap.find(handle.GetId());
    if (it != mHandleToPathMap.end())
    {
        return it->second;
    }
    return kEmptyPath;
}

AssetType AssetDatabase::GetAssetType(AssetHandle handle) const
{
    auto it = mHandleToTypeMap.find(handle.GetId());
    if (it != mHandleToTypeMap.end())
    {
        return it->second;
    }
    return AssetType::Unknown;
}

AssetType AssetDatabase::DeduceAssetType(const std::string& path) const
{
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".jpg" || ext == ".png" || ext == ".dds" || ext == ".tga")
        return AssetType::Texture;
    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
        return AssetType::Model;
    if (ext == ".txt" || ext == ".scene")
        return AssetType::Scene;

    return AssetType::Unknown;
}

bool AssetDatabase::IsValidHandle(AssetHandle handle) const
{
    return mHandleToPathMap.find(handle.GetId()) != mHandleToPathMap.end();
}