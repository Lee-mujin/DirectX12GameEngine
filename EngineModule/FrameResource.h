#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

struct ObjectCBData
{
    DirectX::XMFLOAT4X4 World;
    DirectX::XMFLOAT3 MaterialColor;
    float Shininess;
    int HasNormalMap;
    DirectX::XMFLOAT3 Pad0;
};

struct CameraCBData //카메라 위치
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 Proj;
    DirectX::XMFLOAT3 CameraPosition;
    float Pad0;
};

struct LightCBData
{
    // Directional Light
    DirectX::XMFLOAT3 DirLightDirection;
    float DirLightIntensity;
    DirectX::XMFLOAT3 DirLightColor;
    float Pad0;
    //필드 순서를 재배치해서 float3 + float 짝으로 16바이트씩 맞춤, Pad 정렬 맞추기용 더미
    // Point Light
    DirectX::XMFLOAT3 PointLightPosition;
    float PointLightIntensity;
    DirectX::XMFLOAT3 PointLightColor;
    float PointLightRange;

    int PointLightEnabled;
    DirectX::XMFLOAT3 Pad1;

    // Spot Light
    DirectX::XMFLOAT3 SpotLightPosition;
    float SpotLightIntensity;
    DirectX::XMFLOAT3 SpotLightDirection;
    float SpotLightRange;
    DirectX::XMFLOAT3 SpotLightColor;
    float SpotLightInnerCos; //cos(InnerConeAngle) 값을 CPU에서 미리 계산해서 넘김
    float SpotLightOuterCos; //cos(OuterConeAngle)
    int SpotLightEnabled;
    DirectX::XMFLOAT2 Pad2;
};

static constexpr UINT MAX_BONES = 96;

struct BoneCBData //BoneCB 추가
{
    DirectX::XMFLOAT4X4 BoneMatrices[MAX_BONES];
};

class FrameResource
{
public:
    void Create(ID3D12Device* device, UINT maxObjectCount);

    ComPtr<ID3D12CommandAllocator> commandAllocator;

    ComPtr<ID3D12Resource> objectCB;
    UINT8* objectCBMapped = nullptr;
    UINT objectCBStride = 0;

    ComPtr<ID3D12Resource> cameraCB;
    UINT8* cameraCBMapped = nullptr;

    ComPtr<ID3D12Resource> lightCB;
    UINT8* lightCBMapped = nullptr;

    ComPtr<ID3D12Resource> boneCB;
    UINT8* boneCBMapped = nullptr;
    UINT boneCBStride = 0;

    UINT64 fenceValue = 0; //이 FrameResource를 마지막으로 쓴 프레임의 fence 값
};