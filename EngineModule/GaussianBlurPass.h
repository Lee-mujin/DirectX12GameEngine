#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include "d3dx12.h"

struct BlurCBData
{
    float TexelSize[2] = { 0.0f, 0.0f }; // 1.0f / width, 1.0f / height
    float Direction[2] = { 1.0f, 0.0f }; // (1, 0): 수평, (0, 1): 수직
    float Sigma = 1.0f;                  // 블러 확산 계수 (기본 1.0)
    float Pad[3] = { 0.0f, 0.0f, 0.0f };
};

// 9-Tap 수평/수직 분리 2-Pass 가우시안 블러 풀스크린 패스
class GaussianBlurPass
{
public:
    GaussianBlurPass() = default;
    ~GaussianBlurPass();

    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);

    void Execute(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE inputSrvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE outputRtvHandle,
        UINT width, UINT height,
        float dirX, float dirY,
        float sigma);

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