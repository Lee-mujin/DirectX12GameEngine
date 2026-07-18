#pragma once
#include <cstdint>

// 경로를 직접 들고 다니지 않는 불투명 핸들.
// 실제 경로 조회/캐시는 항상 ResourceManager를 통해서만 이루어진다.
class AssetHandle
{
public:
    AssetHandle() = default;

    bool IsValid() const { return mId != 0; }
    uint32_t GetId() const { return mId; }

    bool operator==(const AssetHandle& other) const { return mId == other.mId; }
    bool operator!=(const AssetHandle& other) const { return mId != other.mId; }

    static const AssetHandle Invalid;

private:
    friend class ResourceManager;
    explicit AssetHandle(uint32_t id) : mId(id) {}

private:
    uint32_t mId = 0;
};