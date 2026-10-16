#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include "d3dx12.h"

enum class ToneMappingType : int
{
    None = 0,
    Reinhard = 1,
    ACES = 2
};

struct ToneMappingCBData
{
    float Exposure = 1.0f;
    int   Type = static_cast<int>(ToneMappingType::ACES);
    float Gamma = 2.2f;
    float BloomIntensity = 0.5f;
};

// HDR RenderTarget 및 Bloom RenderTarget을 읽어 합성, 노출, 톤매핑, 감마 보정을 거쳐 LDR RenderTarget으로 출력하는 풀스크린 패스
class ToneMappingPass
{
public:
    ToneMappingPass() = default;
    ~ToneMappingPass();

    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);

    void Execute(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE hdrSrvHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE bloomSrvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE ldrRtvHandle,
        UINT width, UINT height,
        const ToneMappingCBData& cbData);

private:
    bool CompileShader(const std::wstring& filename, const char* entryPoint, const char* target, ID3DBlob** shaderBlob);
    bool CreateRootSignature(ID3D12Device* device);
    bool CreatePipelineState(ID3D12Device* device, DXGI_FORMAT rtvFormat);
    bool CreateConstantBuffer(ID3D12Device* device);

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  mPSO;

    Microsoft::WRL::ComPtr<ID3DBlob> mVertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> mPixelShader;

    Microsoft::WRL::ComPtr<ID3D12Resource> mConstantBuffer;
    UINT8* mMappedData = nullptr;
};
