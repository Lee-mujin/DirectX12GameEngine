#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "D3D12Renderer.h"
#include "ModelLoader.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include "Animation.h"

void ResourceManager::Initialize(D3D12Renderer* renderer)
{
    mRenderer = renderer;
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
    return mRenderer->LoadTextureFromFile(path);
}

std::shared_ptr<Texture> ResourceManager::GetDefaultWhiteTexture()
{
    if (!mDefaultWhiteTexture)
    {
        mDefaultWhiteTexture = mRenderer->CreateSolidColorTexture(255, 255, 255, 255);
    }
    return mDefaultWhiteTexture;
}

std::shared_ptr<Mesh> ResourceManager::LoadStaticModel(const std::string& path)
{
    return ModelLoader::LoadStaticMesh(mRenderer->GetDevice(), path);
}

std::shared_ptr<SkinnedMesh> ResourceManager::LoadSkinnedModel(const std::string& path, std::vector<std::shared_ptr<Animation>>* outAnimations)
{
    return ModelLoader::LoadSkinnedMesh(mRenderer->GetDevice(), path, outAnimations);
}