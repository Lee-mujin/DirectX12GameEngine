#include "pch.h"
#include "Texture.h"
#include "UploadContext.h"
#include "d3dx12.h"

void Texture::CreateFromPixels(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
    const BYTE* pixels, UINT width, UINT height, const DescriptorHandle& srvHandle, bool srgb)
{
    mSrvHandle = srvHandle;

    // Base Color는 sRGB로 저장되어 있으므로 UNORM_SRGB로 읽어야 선형 PBR 계산이 맞는다.
    // Normal / Mask(ORM) 텍스처는 선형 데이터이므로 UNORM을 유지한다.
    const DXGI_FORMAT textureFormat = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = textureFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    mState = D3D12_RESOURCE_STATE_COMMON;
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        mState, nullptr,
        IID_PPV_ARGS(&mTexture)));

    UINT64 uploadBufferSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadBufferSize);

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&mUploadBuffer)));

    BYTE* mappedData = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(mUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));

    const UINT srcRowPitch = width * 4;
    for (UINT row = 0; row < numRows; ++row)
    {
        memcpy(
            mappedData + footprint.Offset + row * footprint.Footprint.RowPitch,
            pixels + row * srcRowPitch,
            srcRowPitch);
    }
    mUploadBuffer->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = mTexture.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = mUploadBuffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = footprint;

    commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // Copy Queue에서 전이한 상태는 Execute 완료 시 COMMON으로 decay 하므로,
    // SRV 전이는 Graphics Command List에서 수행한다(BindItemTextures 참조).

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(mTexture.Get(), &srvDesc, mSrvHandle.CpuHandle);
}

void Texture::ReleaseUploadBuffer(UploadContext* uploadContext)
{
    if (uploadContext && mUploadBuffer)
    {
        uploadContext->ReleaseWhenComplete(std::move(mUploadBuffer));
    }
}

void Texture::TransitionToRenderState(ID3D12GraphicsCommandList* cmdList)
{
    if (mState == D3D12_RESOURCE_STATE_COMMON)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mTexture.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);
        mState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}