#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>
#include "AssetHandle.h"

enum class AssetType
{
    Unknown,
    Texture,
    Mesh,
    Model,
    Scene
};

//에셋의 경로, GUID, Handle 매핑 정보만 관리하는 전용 데이터베이스
class AssetDatabase
{
public:
    AssetDatabase() = default;

    void Initialize(const std::string& rootPath = "Assets");

    AssetHandle RegisterOrGetHandle(const std::string& path);
    const std::string& GetPath(AssetHandle handle) const;
    AssetType GetAssetType(AssetHandle handle) const;
    AssetType DeduceAssetType(const std::string& path) const;

    bool IsValidHandle(AssetHandle handle) const;

private:
    std::unordered_map<std::string, AssetHandle> mPathToHandleMap;
    std::unordered_map<uint32_t, std::string> mHandleToPathMap;
    std::unordered_map<uint32_t, AssetType> mHandleToTypeMap;

    uint32_t mNextHandleId = 1;
    static const std::string kEmptyPath;
};