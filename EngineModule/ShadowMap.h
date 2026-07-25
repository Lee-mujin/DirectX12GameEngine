#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

class DescriptorAllocator;

class ShadowMap
{
public:
    bool Initialize(ID3D12Device* device, DescriptorAllocator* dsvAllocator, DescriptorAllocator* srvAllocator, UINT size = 2048);

    void Begin(ID3D12GraphicsCommandList* cmd);
    void End(ID3D12GraphicsCommandList* cmd);

    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return mDsvHandle.CpuHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return mSrvHandle.GpuHandle; }
    UINT GetSize() const { return mSize; }

private:
    ComPtr<ID3D12Resource> mDepthTexture;
    DescriptorHandle mDsvHandle;
    DescriptorHandle mSrvHandle;

    UINT mSize = 2048;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT mScissorRect{};
};