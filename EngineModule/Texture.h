#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Texture
{
public:
    void Create(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
        const std::wstring& path,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

    void CreateSolidColor(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
        BYTE r, BYTE g, BYTE b, BYTE a,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return mSrvGpuHandle; }

private:
    void CreateFromPixels(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
        const BYTE* pixels, UINT width, UINT height,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

private:
    ComPtr<ID3D12Resource> mTexture;
    ComPtr<ID3D12Resource> mUploadBuffer; // GPU 복사 끝나도 그냥 계속 들고 있음 (단순화)
    D3D12_GPU_DESCRIPTOR_HANDLE mSrvGpuHandle{};
};