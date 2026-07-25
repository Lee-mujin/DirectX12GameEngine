#include "pch.h"
#include "ShadowMap.h"
#include "DescriptorAllocator.h"
#include "d3dx12.h"

bool ShadowMap::Initialize(ID3D12Device* device, DescriptorAllocator* dsvAllocator, DescriptorAllocator* srvAllocator, UINT size)
{
    mSize = size;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = size;
    desc.Height = size;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    mState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (FAILED(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        mState, &clearValue,
        IID_PPV_ARGS(&mDepthTexture))))
    {
        return false;
    }

    mDsvHandle = dsvAllocator->Allocate();
    if (!mDsvHandle.IsValid())
    {
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(mDepthTexture.Get(), &dsvDesc, mDsvHandle.CpuHandle);

    mSrvHandle = srvAllocator->Allocate();
    if (!mSrvHandle.IsValid())
    {
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(mDepthTexture.Get(), &srvDesc, mSrvHandle.CpuHandle);

    mViewport = { 0.0f, 0.0f, static_cast<float>(size), static_cast<float>(size), 0.0f, 1.0f };
    mScissorRect = { 0, 0, static_cast<LONG>(size), static_cast<LONG>(size) };

    return true;
}

void ShadowMap::Begin(ID3D12GraphicsCommandList* cmd)
{
    if (mState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mDepthTexture.Get(), mState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmd->ResourceBarrier(1, &barrier);
        mState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    cmd->RSSetViewports(1, &mViewport);
    cmd->RSSetScissorRects(1, &mScissorRect);

    cmd->ClearDepthStencilView(mDsvHandle.CpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmd->OMSetRenderTargets(0, nullptr, FALSE, &mDsvHandle.CpuHandle);
}

void ShadowMap::End(ID3D12GraphicsCommandList* cmd)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mDepthTexture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &barrier);
    mState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}