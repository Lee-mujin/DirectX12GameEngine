// ResourceManager.h
#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class Mesh;
class Texture;
class D3D12Renderer;
class Model;

class ResourceManager
{
public:
    void Initialize(D3D12Renderer* renderer);
    std::shared_ptr<Mesh> GetCubeMesh();
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();

    std::shared_ptr<Model> GetOrLoadModel(const std::string& path);

private:
    D3D12Renderer* mRenderer = nullptr;
    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;

    std::unordered_map<std::string, std::shared_ptr<Model>> mModelCache;
};