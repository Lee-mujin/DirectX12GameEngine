#pragma once
#include <memory>
#include <string>
#include <vector>

class Mesh;
class SkinnedMesh;
class Skeleton;
class Animation;
struct ID3D12Device;

class ModelLoader
{
public:
    static std::shared_ptr<Mesh> LoadStaticMesh(ID3D12Device* device, const std::string& path);

    // 스키닝 메시 + 파일 안의 모든 애니메이션 클립을 같은 cgltf_data에서 한 번에 로드
    static std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(
        ID3D12Device* device, const std::string& path,
        std::vector<std::shared_ptr<Animation>>* outAnimations = nullptr);
};