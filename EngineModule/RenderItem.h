#pragma once
#include <d3d12.h>
#include <vector>
#include "Matrix4x4.h"
#include "Vector3.h"

class Mesh;
class SkinnedMesh;
class Texture;

//렌더러와 상위 레벨(Material, Texture, GameObject) 간의 의존성을 격리하는 경량 커맨드 구조체
struct RenderItem
{
    const Mesh* MeshPtr = nullptr;
    const SkinnedMesh* SkinnedMeshPtr = nullptr;

    Matrix4x4 WorldMatrix;

    //서브메시 드로우 파라미터 (DirectX 12 DrawIndexedInstanced 대응)
    UINT IndexStart = 0; //StartIndexLocation
    UINT IndexCount = 0; //IndexCountPerInstance (0이면 메시 전체 인덱스 사용)
    INT BaseVertexLocation = 0; //BaseVertexLocation

    //GPU 핸들 직접 보유 (Material/Texture 객체 직접 접근 배제)
    D3D12_GPU_DESCRIPTOR_HANDLE BaseTextureHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE NormalTextureHandle = { 0 };
    bool HasNormalMap = false;

    // 업로드 직후 텍스처 상태 전이를 위해 렌더러가 단기간 참조하는 포인터.
    // (RenderItem은 프레임 단위로만 사용되며 소유권은 Material이 가진다.)
    const Texture* BaseTexturePtr = nullptr;
    const Texture* NormalTexturePtr = nullptr;

    //머티리얼 상수 속성
    Vector3 BaseColor{ 1.0f, 1.0f, 1.0f };
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float Shininess = 32.0f;

    //애니메이션 스키닝 데이터
    const std::vector<Matrix4x4>* BoneMatrices = nullptr;

    bool IsSkinned() const { return SkinnedMeshPtr != nullptr; }
};