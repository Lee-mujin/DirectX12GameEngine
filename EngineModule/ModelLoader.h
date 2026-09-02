#pragma once

#include <memory>
#include <string>
#include <vector>

class Mesh;
class SkinnedMesh;
class Skeleton;
class Animation;
class UploadContext;
struct ID3D12Device;

class ModelLoader
{
public:
    //정적 메시
    static std::shared_ptr<Mesh> LoadStaticMesh(
        ID3D12Device* device,
        UploadContext* uploadContext,
        const std::string& path);

    //스킨드 메시, 본 계층구조, 애니메이션 트랙 로드
    static std::shared_ptr<SkinnedMesh> LoadSkinnedMesh(
        ID3D12Device* device,
        UploadContext* uploadContext,
        const std::string& path,
        std::vector<std::shared_ptr<Animation>>* outAnimations = nullptr);
};