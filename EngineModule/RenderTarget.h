#pragma once
#include <wrl/client.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

class RenderTarget
{
public:
    void Create(ID3D12Device* device, UINT width, UINT height,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

    void Resize(ID3D12Device* device, UINT width, UINT height);

    ID3D12Resource* GetColorResource() const { return mColorTexture.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return mSrvGpuHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return mRtvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const { return mDsvHandle; }

    UINT GetWidth() const { return mWidth; }
    UINT GetHeight() const { return mHeight; }

private:
    void CreateResources(ID3D12Device* device, UINT width, UINT height);

private:
    ComPtr<ID3D12Resource> mColorTexture;
    ComPtr<ID3D12Resource> mDepthTexture;

    D3D12_CPU_DESCRIPTOR_HANDLE mRtvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE mDsvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE mSrvCpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE mSrvGpuHandle{};

    UINT mWidth = 0;
    UINT mHeight = 0;

    float mClearColor[4] = { 0.0f, 0.2f, 0.6f, 1.0f };
};