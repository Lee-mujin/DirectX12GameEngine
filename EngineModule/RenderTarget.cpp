#include "pch.h"
#include "RenderTarget.h"

void RenderTarget::Create(ID3D12Device* device, UINT width, UINT height,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    mRtvHandle = rtvHandle;
    mDsvHandle = dsvHandle;
    mSrvCpuHandle = srvCpuHandle;
    mSrvGpuHandle = srvGpuHandle;

    CreateResources(device, width, height);
}

void RenderTarget::CreateResources(ID3D12Device* device, UINT width, UINT height)
{
    mWidth = width;
    mHeight = height;

    D3D12_RESOURCE_DESC colorDesc = {};
    colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.DepthOrArraySize = 1;
    colorDesc.MipLevels = 1;
    colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE colorClear = {};
    colorClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    memcpy(colorClear.Color, mClearColor, sizeof(mClearColor));

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &colorDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &colorClear,
        IID_PPV_ARGS(&mColorTexture)));

    device->CreateRenderTargetView(mColorTexture.Get(), nullptr, mRtvHandle);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(mColorTexture.Get(), &srvDesc, mSrvCpuHandle);

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClear = {};
    depthClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthClear.DepthStencil.Depth = 1.0f;

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear,
        IID_PPV_ARGS(&mDepthTexture)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(mDepthTexture.Get(), &dsvDesc, mDsvHandle);
}

void RenderTarget::Resize(ID3D12Device* device, UINT width, UINT height)
{
    if (width == 0 || height == 0 || (width == mWidth && height == mHeight))
    {
        return;
    }

    mColorTexture.Reset();
    mDepthTexture.Reset();

    CreateResources(device, width, height);
}