#pragma once
#include <memory>
#include "Vector3.h"

class Texture;

class Material
{
public:
    Vector3 GetColor() const { return mColor; }
    void SetColor(const Vector3& color) { mColor = color; }

    std::shared_ptr<Texture> GetTexture() const { return mTexture; }
    void SetTexture(std::shared_ptr<Texture> texture) { mTexture = texture; }

private:
    Vector3 mColor{ 1.0f, 1.0f, 1.0f };
    std::shared_ptr<Texture> mTexture;
};