#include "pch.h"
#include "TextureLoader.h"
#include "Texture.h"
#include "UploadContext.h"
#include "DescriptorAllocator.h"

void TextureLoader::Initialize(ID3D12Device* device, DescriptorAllocator* srvAllocator, UploadContext* uploadContext)
{
    mDevice = device;
    mSrvAllocator = srvAllocator;
    mUploadContext = uploadContext;
}

std::shared_ptr<Texture> TextureLoader::LoadTextureFromFile(const std::wstring& path)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

    //렌더러를 우회하여 할당자 관리 기둥으로부터 다이렉트로 슬롯 획득
    if (!mSrvAllocator->Allocate(cpuHandle, gpuHandle)) return nullptr;

    auto texture = std::make_shared<Texture>();

    mUploadContext->Execute([this, &path, cpuHandle, gpuHandle, texture](ID3D12GraphicsCommandList* cmdList) {
        texture->Create(mDevice, cmdList, path, cpuHandle, gpuHandle);
        });

    return texture;
}

std::shared_ptr<Texture> TextureLoader::CreateSolidColorTexture(BYTE r, BYTE g, BYTE b, BYTE a)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

    if (!mSrvAllocator->Allocate(cpuHandle, gpuHandle)) return nullptr;

    auto texture = std::make_shared<Texture>();

    mUploadContext->Execute([this, r, g, b, a, cpuHandle, gpuHandle, texture](ID3D12GraphicsCommandList* cmdList) {
        texture->CreateSolidColor(mDevice, cmdList, r, g, b, a, cpuHandle, gpuHandle);
        });

    return texture;
}