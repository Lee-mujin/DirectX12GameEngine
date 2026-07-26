#pragma once
#include <d3d12.h>
#include <vector>
#include "Matrix4x4.h"
#include "Vector3.h"

class Mesh;
class SkinnedMesh;

//렌더러와 상위 레벨(Material, Texture, GameObject) 간의 의존성을 격리하는 경량 커맨드 구조체
struct RenderItem
{
    const Mesh* MeshPtr = nullptr;
    const SkinnedMesh* SkinnedMeshPtr = nullptr;

    Matrix4x4 WorldMatrix;

    // GPU 핸들 직접 보유 (Material/Texture 객체 직접 접근 배제)
    D3D12_GPU_DESCRIPTOR_HANDLE BaseTextureHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE NormalTextureHandle = { 0 };
    bool HasNormalMap = false;

    // 머티리얼 상수 속성
    Vector3 BaseColor{ 1.0f, 1.0f, 1.0f };
    float Shininess = 32.0f;

    // 애니메이션 스키닝 데이터
    const std::vector<Matrix4x4>* BoneMatrices = nullptr;

    bool IsSkinned() const { return SkinnedMeshPtr != nullptr; }
};