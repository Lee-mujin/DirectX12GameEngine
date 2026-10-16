#pragma once
#include <string>
#include <memory>
#include <vector>
#include <d3d12.h>

class Texture;
class UploadContext;
class DescriptorAllocator;

class TextureLoader
{
public:
    TextureLoader() = default;
    ~TextureLoader() = default;

    void Initialize(ID3D12Device* device, DescriptorAllocator* srvAllocator, UploadContext* uploadContext);

    std::shared_ptr<Texture> LoadTextureFromFile(const std::wstring& path);
    std::shared_ptr<Texture> LoadTextureFromMemory(const void* data, size_t sizeBytes);
    std::shared_ptr<Texture> CreateTextureFromRawRGBA(const void* pixels, UINT width, UINT height);
    std::shared_ptr<Texture> CreateSolidColorTexture(BYTE r, BYTE g, BYTE b, BYTE a);

private:
    bool DecodeWICFile(const std::wstring& path, std::vector<BYTE>& outPixels, UINT& outWidth, UINT& outHeight);
    bool DecodeWICMemory(const void* data, size_t sizeBytes, std::vector<BYTE>& outPixels, UINT& outWidth, UINT& outHeight);

private:
    ID3D12Device* mDevice = nullptr;
    DescriptorAllocator* mSrvAllocator = nullptr;
    UploadContext* mUploadContext = nullptr;
};