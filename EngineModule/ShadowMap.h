#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "DescriptorAllocator.h"

class ShadowMap
{
public:
    ShadowMap() = default;

    bool Initialize(ID3D12Device* device, DescriptorAllocator* dsvAllocator, DescriptorAllocator* srvAllocator, UINT size = 2048);

    //특정 Cascade Layer (0~2)의 DSV로 바인딩
    void BeginPass(ID3D12GraphicsCommandList* commandList, UINT cascadeIndex);
    void EndPass(ID3D12GraphicsCommandList* commandList);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return mSrvGpuHandle; }
    UINT GetSize() const { return mSize; }

private:
    UINT mSize = 2048;
    static constexpr UINT kCascadeCount = 3; //Cascade 3개 (Near, Mid, Far)

    Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMapArray;

    // Layer별 DSV 핸들 (3개)
    D3D12_CPU_DESCRIPTOR_HANDLE mDsvCpuHandles[kCascadeCount]{};

    // Texture2DArray 전체를 가리키는 SRV
    D3D12_CPU_DESCRIPTOR_HANDLE mSrvCpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE mSrvGpuHandle{};

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT mScissorRect{};
};