#pragma once
#include <memory>
#include "Material.h"

//원본 Material을 공유하면서 개별 인스턴스 파라미터를 오버라이드하는 MaterialInstance
class MaterialInstance
{
public:
    MaterialInstance() = default;
    explicit MaterialInstance(std::shared_ptr<Material> parentMaterial)
        : mParentMaterial(parentMaterial)
    {
        if (mParentMaterial)
        {
            mBaseColor = mParentMaterial->GetBaseColor();
            mMetallic = mParentMaterial->GetMetallic();
            mRoughness = mParentMaterial->GetRoughness();
            mShininess = mParentMaterial->GetShininess();
            mTexture = mParentMaterial->GetTexture();
            mNormalTexture = mParentMaterial->GetNormalTexture();
        }
    }

    void SetParent(std::shared_ptr<Material> parent)
    {
        mParentMaterial = parent;
        if (mParentMaterial)
        {
            mBaseColor = mParentMaterial->GetBaseColor();
            mMetallic = mParentMaterial->GetMetallic();
            mRoughness = mParentMaterial->GetRoughness();
            mShininess = mParentMaterial->GetShininess();
            mTexture = mParentMaterial->GetTexture();
            mNormalTexture = mParentMaterial->GetNormalTexture();
        }
    }

    std::shared_ptr<Material> GetParent() const { return mParentMaterial; }

    //개별 속성 오버라이드
    void SetBaseColor(const Vector3& color) { mBaseColor = color; mOverrideColor = true; }
    void SetMetallic(float metallic) { mMetallic = metallic; mOverrideMetallic = true; }
    void SetRoughness(float roughness) { mRoughness = roughness; mOverrideRoughness = true; }
    void SetShininess(float shininess) { mShininess = shininess; mOverrideShininess = true; }
    void SetTexture(std::shared_ptr<Texture> texture) { mTexture = texture; mOverrideTexture = true; }
    void SetNormalTexture(std::shared_ptr<Texture> texture) { mNormalTexture = texture; mOverrideNormal = true; }

    //값 조회 (오버라이드된 값이 없으면 부모 Material 값 반환)
    Vector3 GetBaseColor() const
    {
        if (mOverrideColor) return mBaseColor;
        return mParentMaterial ? mParentMaterial->GetBaseColor() : mBaseColor;
    }

    float GetMetallic() const
    {
        if (mOverrideMetallic) return mMetallic;
        return mParentMaterial ? mParentMaterial->GetMetallic() : mMetallic;
    }

    float GetRoughness() const
    {
        if (mOverrideRoughness) return mRoughness;
        return mParentMaterial ? mParentMaterial->GetRoughness() : mRoughness;
    }

    float GetShininess() const
    {
        if (mOverrideShininess) return mShininess;
        return mParentMaterial ? mParentMaterial->GetShininess() : mShininess;
    }

    std::shared_ptr<Texture> GetTexture() const
    {
        if (mOverrideTexture) return mTexture;
        return mParentMaterial ? mParentMaterial->GetTexture() : mTexture;
    }

    std::shared_ptr<Texture> GetNormalTexture() const
    {
        if (mOverrideNormal) return mNormalTexture;
        return mParentMaterial ? mParentMaterial->GetNormalTexture() : mNormalTexture;
    }

private:
    std::shared_ptr<Material> mParentMaterial;

    Vector3 mBaseColor{ 1.0f, 1.0f, 1.0f };
    float mMetallic = 0.0f;
    float mRoughness = 0.5f;
    float mShininess = 32.0f;
    std::shared_ptr<Texture> mTexture;
    std::shared_ptr<Texture> mNormalTexture;

    bool mOverrideColor = false;
    bool mOverrideMetallic = false;
    bool mOverrideRoughness = false;
    bool mOverrideShininess = false;
    bool mOverrideTexture = false;
    bool mOverrideNormal = false;
};