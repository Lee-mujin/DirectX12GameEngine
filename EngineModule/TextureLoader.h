#pragma once
#include <string>
#include <memory>
#include <d3d12.h>

class Texture;
class UploadContext;
class DescriptorAllocator; //렌더러 대신 디스크립터 할당자를 직접 포워드 선언

class TextureLoader
{
public:
    TextureLoader() = default;
    ~TextureLoader() = default;

    //Renderer 전체가 아닌 필요한 하위 기둥 3개만 주입
    void Initialize(ID3D12Device* device, DescriptorAllocator* srvAllocator, UploadContext* uploadContext);

    std::shared_ptr<Texture> LoadTextureFromFile(const std::wstring& path);
    std::shared_ptr<Texture> CreateSolidColorTexture(BYTE r, BYTE g, BYTE b, BYTE a);

private:
    ID3D12Device* mDevice = nullptr; //직접 디바이스 사용
    DescriptorAllocator* mSrvAllocator = nullptr; //직접 디스크립터 슬롯 할당 호출
    UploadContext* mUploadContext = nullptr;
};