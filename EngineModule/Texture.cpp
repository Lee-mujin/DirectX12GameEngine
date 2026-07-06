#include "pch.h"
#include "Texture.h"
#include <wincodec.h>
#include <vector>

namespace
{
    ComPtr<IWICImagingFactory> GetWicFactory()
    {
        static ComPtr<IWICImagingFactory> factory;
        if (!factory)
        {
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            ThrowIfFailed(CoCreateInstance(
                CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory)));
        }
        return factory;
    }
}

void Texture::Create(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
    const std::wstring& path,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    ComPtr<IWICImagingFactory> wicFactory = GetWicFactory();

    ComPtr<IWICBitmapDecoder> decoder;
    ThrowIfFailed(wicFactory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder));

    ComPtr<IWICBitmapFrameDecode> frame;
    ThrowIfFailed(decoder->GetFrame(0, &frame));

    ComPtr<IWICFormatConverter> converter;
    ThrowIfFailed(wicFactory->CreateFormatConverter(&converter));
    ThrowIfFailed(converter->Initialize(
        frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom));

    UINT width = 0, height = 0;
    ThrowIfFailed(converter->GetSize(&width, &height));

    std::vector<BYTE> pixels(static_cast<size_t>(width) * height * 4);
    ThrowIfFailed(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data()));

    CreateFromPixels(device, commandList, pixels.data(), width, height, srvCpuHandle, srvGpuHandle);
}

void Texture::CreateSolidColor(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
    BYTE r, BYTE g, BYTE b, BYTE a,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    BYTE pixel[4] = { r, g, b, a };
    CreateFromPixels(device, commandList, pixel, 1, 1, srvCpuHandle, srvGpuHandle);
}

void Texture::CreateFromPixels(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
    const BYTE* pixels, UINT width, UINT height,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
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

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mTexture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(mTexture.Get(), &srvDesc, srvCpuHandle);

    mSrvGpuHandle = srvGpuHandle;
}