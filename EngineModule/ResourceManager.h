#pragma once
#include <memory>
#include <string>

class Mesh;
class Texture;
class D3D12Renderer;

class ResourceManager
{
public:
    void Initialize(D3D12Renderer* renderer);
    std::shared_ptr<Mesh> GetCubeMesh();
    std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
    std::shared_ptr<Texture> GetDefaultWhiteTexture();
    std::shared_ptr<Mesh> LoadStaticModel(const std::string& path);
	std::shared_ptr<Mesh> LoadSkinnedModel(const std::string& path);
private:
    D3D12Renderer* mRenderer = nullptr;
    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Texture> mDefaultWhiteTexture;
};