#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    //순수 픽셀 데이터와 핸들만 받아 GPU 리소스를 생성
    void CreateFromPixels(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
        const BYTE* pixels, UINT width, UINT height, const DescriptorHandle& srvHandle);

    void TransitionToRenderState(ID3D12GraphicsCommandList* cmdList);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return mSrvHandle.GpuHandle; }
    const DescriptorHandle& GetDescriptorHandle() const { return mSrvHandle; }

private:
    ComPtr<ID3D12Resource> mTexture;
    ComPtr<ID3D12Resource> mUploadBuffer;
    DescriptorHandle mSrvHandle;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};