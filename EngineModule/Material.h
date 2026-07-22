#pragma once
#include <memory>
#include "Vector3.h"
#include "AssetHandle.h"

class Texture;

class Material
{
public:
    Material() = default;
    ~Material() = default;

    // Base Color Texture
    void SetBaseColorTexture(std::shared_ptr<Texture> texture, AssetHandle handle = AssetHandle())
    {
        mBaseColorTexture = texture;
        mBaseColorHandle = handle;
    }
    std::shared_ptr<Texture> GetBaseColorTexture() const { return mBaseColorTexture; }
    AssetHandle GetBaseColorHandle() const { return mBaseColorHandle; }

    //하위 호환성을 위한 래퍼 메서드
    void SetTexture(std::shared_ptr<Texture> texture, AssetHandle handle = AssetHandle())
    {
        SetBaseColorTexture(texture, handle);
    }
    std::shared_ptr<Texture> GetTexture() const
    {
        return GetBaseColorTexture();
    }

    // Normal Texture
    void SetNormalTexture(std::shared_ptr<Texture> texture, AssetHandle handle = AssetHandle())
    {
        mNormalTexture = texture;
        mNormalHandle = handle;
    }
    std::shared_ptr<Texture> GetNormalTexture() const { return mNormalTexture; }
    AssetHandle GetNormalHandle() const { return mNormalHandle; }

    // PBR & Material Properties
    const Vector3& GetBaseColor() const { return mBaseColor; }
    void SetBaseColor(const Vector3& color) { mBaseColor = color; }

    float GetMetallic() const { return mMetallic; }
    void SetMetallic(float metallic) { mMetallic = metallic; }

    float GetRoughness() const { return mRoughness; }
    void SetRoughness(float roughness) { mRoughness = roughness; }

    float GetShininess() const { return mShininess; }
    void SetShininess(float shininess) { mShininess = shininess; }

private:
    Vector3 mBaseColor{ 1.0f, 1.0f, 1.0f };
    float mMetallic = 0.0f;
    float mRoughness = 0.5f;
    float mShininess = 32.0f;

    std::shared_ptr<Texture> mBaseColorTexture;
    AssetHandle mBaseColorHandle;

    std::shared_ptr<Texture> mNormalTexture;
    AssetHandle mNormalHandle;
};