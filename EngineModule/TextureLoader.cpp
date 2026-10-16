#include "pch.h"
#include "TextureLoader.h"
#include "Texture.h"
#include "UploadContext.h"
#include "DescriptorAllocator.h"
#include <wincodec.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

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

void TextureLoader::Initialize(ID3D12Device* device, DescriptorAllocator* srvAllocator, UploadContext* uploadContext)
{
    mDevice = device;
    mSrvAllocator = srvAllocator;
    mUploadContext = uploadContext;
}

bool TextureLoader::DecodeWICFile(const std::wstring& path, std::vector<BYTE>& outPixels, UINT& outWidth, UINT& outHeight)
{
    ComPtr<IWICImagingFactory> wicFactory = GetWicFactory();

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
        return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame)))
        return false;

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(wicFactory->CreateFormatConverter(&converter)))
        return false;

    if (FAILED(converter->Initialize(
        frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        return false;

    if (FAILED(converter->GetSize(&outWidth, &outHeight)))
        return false;

    outPixels.resize(static_cast<size_t>(outWidth) * outHeight * 4);
    return SUCCEEDED(converter->CopyPixels(nullptr, outWidth * 4, static_cast<UINT>(outPixels.size()), outPixels.data()));
}

bool TextureLoader::DecodeWICMemory(const void* data, size_t sizeBytes, std::vector<BYTE>& outPixels, UINT& outWidth, UINT& outHeight)
{
    if (!data || sizeBytes == 0) return false;

    ComPtr<IWICImagingFactory> wicFactory = GetWicFactory();
    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(static_cast<const BYTE*>(data), static_cast<UINT>(sizeBytes)));
    if (!stream) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder)))
        return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame)))
        return false;

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(wicFactory->CreateFormatConverter(&converter)))
        return false;

    if (FAILED(converter->Initialize(
        frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        return false;

    if (FAILED(converter->GetSize(&outWidth, &outHeight)))
        return false;

    outPixels.resize(static_cast<size_t>(outWidth) * outHeight * 4);
    return SUCCEEDED(converter->CopyPixels(nullptr, outWidth * 4, static_cast<UINT>(outPixels.size()), outPixels.data()));
}

std::shared_ptr<Texture> TextureLoader::LoadTextureFromMemory(const void* data, size_t sizeBytes)
{
    std::vector<BYTE> pixels;
    UINT width = 0, height = 0;

    if (!DecodeWICMemory(data, sizeBytes, pixels, width, height))
        return nullptr;

    DescriptorHandle srvHandle = mSrvAllocator->Allocate();
    if (!srvHandle.IsValid())
        return nullptr;

    auto texture = std::make_shared<Texture>();

    mUploadContext->Execute([this, pixels = std::move(pixels), width, height, srvHandle, texture](ID3D12GraphicsCommandList* cmdList) {
        texture->CreateFromPixels(mDevice, cmdList, pixels.data(), width, height, srvHandle);
        });

    return texture;
}

std::shared_ptr<Texture> TextureLoader::CreateTextureFromRawRGBA(const void* pixels, UINT width, UINT height)
{
    if (!pixels || width == 0 || height == 0) return nullptr;

    DescriptorHandle srvHandle = mSrvAllocator->Allocate();
    if (!srvHandle.IsValid())
        return nullptr;

    auto texture = std::make_shared<Texture>();
    std::vector<BYTE> pixelData(static_cast<const BYTE*>(pixels), static_cast<const BYTE*>(pixels) + (width * height * 4));

    mUploadContext->Execute([this, pixelData = std::move(pixelData), width, height, srvHandle, texture](ID3D12GraphicsCommandList* cmdList) {
        texture->CreateFromPixels(mDevice, cmdList, pixelData.data(), width, height, srvHandle);
        });

    return texture;
}

std::shared_ptr<Texture> TextureLoader::LoadTextureFromFile(const std::wstring& path)
{
    std::vector<BYTE> pixels;
    UINT width = 0, height = 0;

    //CPU에서 WIC 파일 로딩
    if (!DecodeWICFile(path, pixels, width, height))
        return nullptr;

    DescriptorHandle srvHandle = mSrvAllocator->Allocate();
    if (!srvHandle.IsValid())
        return nullptr;

    auto texture = std::make_shared<Texture>();

    mUploadContext->Execute([this, pixels = std::move(pixels), width, height, srvHandle, texture](ID3D12GraphicsCommandList* cmdList) {
        texture->CreateFromPixels(mDevice, cmdList, pixels.data(), width, height, srvHandle);
        });

    return texture;
}

std::shared_ptr<Texture> TextureLoader::CreateSolidColorTexture(BYTE r, BYTE g, BYTE b, BYTE a)
{
    DescriptorHandle srvHandle = mSrvAllocator->Allocate();
    if (!srvHandle.IsValid())
        return nullptr;

    auto texture = std::make_shared<Texture>();
    std::vector<BYTE> pixel = { r, g, b, a };

    mUploadContext->Execute([this, pixel = std::move(pixel), srvHandle, texture](ID3D12GraphicsCommandList* cmdList) {
        texture->CreateFromPixels(mDevice, cmdList, pixel.data(), 1, 1, srvHandle);
        });

    return texture;
}