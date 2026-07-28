#include "pch.h"
#include "ShadowMap.h"
#include "d3dx12.h"

bool ShadowMap::Initialize(ID3D12Device* device, DescriptorAllocator* dsvAllocator, DescriptorAllocator* srvAllocator, UINT size)
{
    mSize = size;
    mViewport = { 0.0f, 0.0f, static_cast<float>(mSize), static_cast<float>(mSize), 0.0f, 1.0f };
    mScissorRect = { 0, 0, static_cast<LONG>(mSize), static_cast<LONG>(mSize) };

    // 1. Texture2DArray 텍스처 리소스 생성 (DepthOrArraySize = 3)
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = mSize;
    texDesc.Height = mSize;
    texDesc.DepthOrArraySize = kCascadeCount; //3개의 레이어
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // SRV/DSV 겸용 Typeless
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue,
        IID_PPV_ARGS(&mShadowMapArray)));

    // 2. 각 Layer별 DSV 3개 생성
    for (UINT i = 0; i < kCascadeCount; ++i)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.ArraySize = 1; // 단일 슬라이스만 렌더링 지정
        dsvDesc.Texture2DArray.FirstArraySlice = i;
        dsvDesc.Texture2DArray.MipSlice = 0;

        DescriptorHandle dsvHandle = dsvAllocator->Allocate();
        mDsvCpuHandles[i] = dsvHandle.CpuHandle;
        device->CreateDepthStencilView(mShadowMapArray.Get(), &dsvDesc, mDsvCpuHandles[i]);
    }

    // 3. Texture2DArray 전체를 참조하는 SRV 1개 생성
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = kCascadeCount; //3개 레이어 통째로 바인딩

    DescriptorHandle srvHandle = srvAllocator->Allocate();
    mSrvCpuHandle = srvHandle.CpuHandle;
    mSrvGpuHandle = srvHandle.GpuHandle;

    device->CreateShaderResourceView(mShadowMapArray.Get(), &srvDesc, mSrvCpuHandle);

    return true;
}

void ShadowMap::BeginPass(ID3D12GraphicsCommandList* commandList, UINT cascadeIndex)
{
    commandList->RSSetViewports(1, &mViewport);
    commandList->RSSetScissorRects(1, &mScissorRect);

    // 0번 패스 시작 시에만 리소스 상태를 DEPTH_WRITE로 전이
    if (cascadeIndex == 0)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mShadowMapArray.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(1, &barrier);
    }

    // 해당 Cascade Layer의 DSV 바인딩 및 Clear
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = mDsvCpuHandles[cascadeIndex];
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void ShadowMap::EndPass(ID3D12GraphicsCommandList* commandList)
{
    // 렌더링 완료 후 Pixel Shader Read 상태로 전이
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mShadowMapArray.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
    commandList->ResourceBarrier(1, &barrier);
}