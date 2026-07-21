// Material.h
#pragma once
#include <memory>
#include "Vector3.h"

class Texture;

class Material
{
public:
    //Base Color
    Vector3 GetBaseColor() const { return mBaseColor; }
    void SetBaseColor(const Vector3& color) { mBaseColor = color; }

    std::shared_ptr<Texture> GetBaseColorTexture() const { return mBaseColorTexture; }
    void SetBaseColorTexture(std::shared_ptr<Texture> texture) { mBaseColorTexture = texture; }

    //PBR 스칼라 파라미터
    float GetMetallic() const { return mMetallic; }
    void SetMetallic(float value) { mMetallic = value; }

    float GetRoughness() const { return mRoughness; }
    void SetRoughness(float value) { mRoughness = value; }

    std::shared_ptr<Texture> GetNormalTexture() const { return mNormalTexture; }
    void SetNormalTexture(std::shared_ptr<Texture> texture) { mNormalTexture = texture; }

    //기존 Phong 파이프라인 호환용
    float GetShininess() const { return mShininess; }
    void SetShininess(float shininess) { mShininess = shininess; }

    //하위 호환
    Vector3 GetColor() const { return mBaseColor; }
    void SetColor(const Vector3& color) { mBaseColor = color; }
    std::shared_ptr<Texture> GetTexture() const { return mBaseColorTexture; }
    void SetTexture(std::shared_ptr<Texture> texture) { mBaseColorTexture = texture; }

private:
    Vector3 mBaseColor{ 1.0f, 1.0f, 1.0f };
    std::shared_ptr<Texture> mBaseColorTexture;

    float mMetallic = 0.0f;
    float mRoughness = 0.5f;
    std::shared_ptr<Texture> mNormalTexture; //지금은 셰이더 미사용, 데이터만 보관

    float mShininess = 32.0f;
};