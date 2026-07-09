#pragma once
#include <memory>
#include <string>

class Mesh;
class SkinnedMesh;
struct ID3D12Device;

class ModelLoader
{
public:
    static std::shared_ptr<Mesh> LoadStaticMesh(ID3D12Device* device, const std::string& path);
    static std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(ID3D12Device* device, const std::string& path);
};