#pragma once
#include <memory>
#include <string>

class Mesh;
class SkinnedMesh;
class Skeleton;
class Animation;
struct ID3D12Device;

class ModelLoader
{
public:
    static std::shared_ptr<Mesh> LoadStaticMesh(ID3D12Device* device, const std::string& path);
    static std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(ID3D12Device* device, const std::string& path);
    static std::shared_ptr<Animation> LoadAnimation(const std::string& path, std::shared_ptr<Skeleton> skeleton);
};