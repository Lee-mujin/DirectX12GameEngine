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
    float Metallic;
    float Roughness;
    int HasNormalMap;
    float Pad0;
};

struct CameraCBData
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 Proj;
    DirectX::XMFLOAT3 CameraPosition;
    float Pad0;
};

//CSM 캐스케이드 개수
static constexpr UINT kNumCascades = 3;

struct LightCBData
{
    // Directional Light
    DirectX::XMFLOAT3 DirLightDirection;
    float DirLightIntensity;
    DirectX::XMFLOAT3 DirLightColor;
    float Pad0;

    //Directional Light 전용 Cascade ViewProj 행렬 3개
    DirectX::XMFLOAT4X4 WorldLightViewProj[kNumCascades];

    //Cascade 분할 거리 (x: Near, y: Mid, z: Far)
    DirectX::XMFLOAT4 CascadeSplits;

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
    float SpotLightInnerCos;
    float SpotLightOuterCos;
    int SpotLightEnabled;

    //Spot Light ViewProj 행렬
    DirectX::XMFLOAT4X4 SpotLightViewProj;
};

struct ShadowCBData
{
    DirectX::XMFLOAT4X4 WorldLightViewProj;
};

static constexpr UINT MAX_BONES = 96;

struct BoneCBData
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

    ComPtr<ID3D12Resource> shadowCB;
    UINT8* shadowCBMapped = nullptr;
    UINT shadowCBStride = 0;

    UINT64 fenceValue = 0;
};