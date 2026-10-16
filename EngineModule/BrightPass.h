#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include "d3dx12.h"

struct BrightPassCBData
{
    float Threshold = 0.6f;
    float SoftKnee = 0.5f;
    float Pad0 = 0.0f;
    float Pad1 = 0.0f;
};

// HDR RenderTarget에서 임계값을 초과하는 고광도 픽셀을 Soft-Knee 커브로 추출하는 풀스크린 패스
class BrightPass
{
public:
    BrightPass() = default;
    ~BrightPass();

    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);

    void Execute(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE hdrSrvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE bloomRtvHandle,
        UINT width, UINT height,
        const BrightPassCBData& cbData);

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
