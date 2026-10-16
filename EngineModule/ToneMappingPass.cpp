#include "pch.h"
#include "ToneMappingPass.h"
#include <d3dcompiler.h>
#include <filesystem>

ToneMappingPass::~ToneMappingPass()
{
    if (mConstantBuffer && mMappedData)
    {
        mConstantBuffer->Unmap(0, nullptr);
        mMappedData = nullptr;
    }
}

bool ToneMappingPass::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat)
{
    if (!device) return false;

    // 1. 셰이더 컴파일
    std::wstring shaderPath = L"Shaders/ToneMapping.hlsl";
    if (!std::filesystem::exists(shaderPath))
    {
        shaderPath = L"../GameEngine/Shaders/ToneMapping.hlsl";
    }

    if (!CompileShader(shaderPath, "VSMain", "vs_5_0", &mVertexShader))
    {
        OutputDebugStringA("[ToneMappingPass] Failed to compile VSMain\n");
        return false;
    }

    if (!CompileShader(shaderPath, "PSMain", "ps_5_0", &mPixelShader))
    {
        OutputDebugStringA("[ToneMappingPass] Failed to compile PSMain\n");
        return false;
    }

    // 2. 루트 시그니처 생성
    if (!CreateRootSignature(device)) return false;

    // 3. 파이프라인 상태 객체(PSO) 생성
    if (!CreatePipelineState(device, rtvFormat)) return false;

    // 4. 상수 버퍼 생성
    if (!CreateConstantBuffer(device)) return false;

    return true;
}

bool ToneMappingPass::CompileShader(const std::wstring& filename, const char* entryPoint, const char* target, ID3DBlob** shaderBlob)
{
    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(
        filename.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        compileFlags,
        0,
        shaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }

    return true;
}

bool ToneMappingPass::CreateRootSignature(ID3D12Device* device)
{
    // Slot 0: ConstantBufferView (b0 - ToneMappingCB)
    // Slot 1: DescriptorTable (t0 - HDRTexture SRV)
    // Slot 2: DescriptorTable (t1 - BloomTexture SRV)
    CD3DX12_ROOT_PARAMETER rootParams[3];
    rootParams[0].InitAsConstantBufferView(0);

    CD3DX12_DESCRIPTOR_RANGE srvRangeHDR;
    srvRangeHDR.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParams[1].InitAsDescriptorTable(1, &srvRangeHDR, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_DESCRIPTOR_RANGE srvRangeBloom;
    srvRangeBloom.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    rootParams[2].InitAsDescriptorTable(1, &srvRangeBloom, D3D12_SHADER_VISIBILITY_PIXEL);

    // Static Sampler 0: s0 (Linear Filter, Clamp Address Mode)
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(_countof(rootParams), rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
    return SUCCEEDED(hr);
}

bool ToneMappingPass::CreatePipelineState(ID3D12Device* device, DXGI_FORMAT rtvFormat)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 }; // Full-screen triangle: SV_VertexID 사용
    psoDesc.pRootSignature = mRootSignature.Get();

    if (mVertexShader) psoDesc.VS = { mVertexShader->GetBufferPointer(), mVertexShader->GetBufferSize() };
    if (mPixelShader)  psoDesc.PS = { mPixelShader->GetBufferPointer(), mPixelShader->GetBufferSize() };

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 컬링 없음
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // Opaque overwrite

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = rtvFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
    return SUCCEEDED(hr);
}

bool ToneMappingPass::CreateConstantBuffer(ID3D12Device* device)
{
    UINT cbSize = (sizeof(ToneMappingCBData) + 255) & ~255; // 256바이트 정렬

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mConstantBuffer)
    );

    if (FAILED(hr)) return false;

    CD3DX12_RANGE readRange(0, 0); // CPU 읽기 비활성화
    hr = mConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mMappedData));
    return SUCCEEDED(hr);
}

void ToneMappingPass::Execute(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_DESCRIPTOR_HANDLE hdrSrvHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE bloomSrvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE ldrRtvHandle,
    UINT width, UINT height,
    const ToneMappingCBData& cbData)
{
    if (!commandList || !mRootSignature || !mPSO) return;

    // 1. 상수 버퍼 데이터 업데이트
    if (mMappedData)
    {
        memcpy(mMappedData, &cbData, sizeof(ToneMappingCBData));
    }

    // 2. 뷰포트 및 시저렉트 설정
    D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &scissor);

    // 3. 렌더 타깃 설정 (LDR 뷰포트 타깃, 깊이 버퍼 사용 안함)
    commandList->OMSetRenderTargets(1, &ldrRtvHandle, FALSE, nullptr);

    // 4. 파이프라인 및 루트 시그니처 바인딩
    commandList->SetGraphicsRootSignature(mRootSignature.Get());
    commandList->SetPipelineState(mPSO.Get());

    // 5. 파라미터 바인딩
    commandList->SetGraphicsRootConstantBufferView(0, mConstantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, hdrSrvHandle);
    commandList->SetGraphicsRootDescriptorTable(2, bloomSrvHandle);

    // 6. Full-Screen Triangle 드로우
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}
