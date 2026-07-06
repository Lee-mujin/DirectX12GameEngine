#include "pch.h"
#include "D3D12Renderer.h"
#include "MathUtil.h"
#include "DirectionalLight.h"
#include "Texture.h"
#include "FrameResource.h"
#include "PointLight.h"
#include "SpotLight.h"

using namespace DirectX;

namespace
{
    XMMATRIX ToXM(const Matrix4x4& m)
    {
        return XMMatrixSet(
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
    }
}

bool D3D12Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    mHwnd = hwnd;
    mWidth = width;
    mHeight = height;

    CreateFactory();
    CreateDevice();
    CreateCommandQueue();
    CreateFrameResources();
    CreateCommandList();
    CreateFence();
    CreateSwapChain();
    CreateDescriptorHeaps();
    CreateSrvHeap();
    CreateRenderTargetViews();
    CreateDepthStencil();
    SetupViewport();

    CreateShader();
    CreateRootSignature();
    CreatePSO();

    return true;
}

void D3D12Renderer::CreateFactory()
{
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
}

void D3D12Renderer::CreateDevice()
{
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
}

void D3D12Renderer::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
}

void D3D12Renderer::CreateFrameResources()
{
    for (auto& fr : mFrameResources)
    {
        fr.Create(mDevice.Get(), kMaxObjectsPerFrame);
    }
}

void D3D12Renderer::CreateCommandList()
{
    ThrowIfFailed(mDevice->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        mFrameResources[0].commandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&mCommandList)));

    mCommandList->Close();
}

void D3D12Renderer::CreateFence()
{
    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceValue = 0;

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void D3D12Renderer::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.BufferCount = kFrameCount;
    desc.Width = mWidth;
    desc.Height = mHeight;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(mFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mHwnd, &desc, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(mFactory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain1.As(&mSwapChain));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void D3D12Renderer::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.NumDescriptors = kFrameCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&mRTVHeap)));
    mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&mDSVHeap)));
    mDSVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void D3D12Renderer::CreateSrvHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = kMaxSrvCount;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mSrvHeap)));
    mSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

bool D3D12Renderer::AllocateSrvSlot(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu, D3D12_GPU_DESCRIPTOR_HANDLE& outGpu)
{
    if (mSrvUsedCount >= kMaxSrvCount)
    {
        return false;
    }

    outCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart(), mSrvUsedCount, mSrvDescriptorSize);
    outGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvHeap->GetGPUDescriptorHandleForHeapStart(), mSrvUsedCount, mSrvDescriptorSize);
    mSrvUsedCount++;
    return true;
}

void D3D12Renderer::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < kFrameCount; ++i)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])));
        mDevice->CreateRenderTargetView(mBackBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, mRTVDescriptorSize);
    }
}

void D3D12Renderer::CreateDepthStencil()
{
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = mWidth;
    depthDesc.Height = mHeight;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(mDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&mDepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mDSVHeap->GetCPUDescriptorHandleForHeapStart());
}

void D3D12Renderer::SetupViewport()
{
    mViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0f, 1.0f };
    mScissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
}

void D3D12Renderer::CreateShader()
{
    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(L"Shaders/Default.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &mVertexShader, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        ThrowIfFailed(hr);
    }

    hr = D3DCompileFromFile(L"Shaders/Default.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &mPixelShader, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        ThrowIfFailed(hr);
    }
}

void D3D12Renderer::CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParams[4];
    rootParams[0].InitAsConstantBufferView(0); // ObjectCB
    rootParams[1].InitAsConstantBufferView(1); // CameraCB
    rootParams[2].InitAsConstantBufferView(2); // LightCB
    rootParams[3].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(_countof(rootParams), rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signatureBlob, errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
    ThrowIfFailed(mDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void D3D12Renderer::CreatePSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { mVertexShader->GetBufferPointer(), mVertexShader->GetBufferSize() };
    psoDesc.PS = { mPixelShader->GetBufferPointer(), mPixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void D3D12Renderer::BeginFrame(const Camera& camera, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    if (fr.fenceValue != 0 && mFence->GetCompletedValue() < fr.fenceValue)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(fr.fenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    ThrowIfFailed(fr.commandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(fr.commandAllocator.Get(), nullptr));

    ID3D12DescriptorHeap* heaps[] = { mSrvHeap.Get() };
    mCommandList->SetDescriptorHeaps(1, heaps);

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    CD3DX12_RESOURCE_BARRIER barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(
        mBackBuffers[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrierToRT);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRTVDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDSVHeap->GetCPUDescriptorHandleForHeapStart());

    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    mCommandList->ClearRenderTargetView(rtvHandle, mClearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetPipelineState(mPSO.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Camera CB
    CameraCBData camData;
    XMStoreFloat4x4(&camData.View, XMMatrixTranspose(ToXM(camera.GetView())));
    XMStoreFloat4x4(&camData.Proj, XMMatrixTranspose(ToXM(camera.GetProjection())));

    Vector3 camPos = camera.GetPosition();
    camData.CameraPosition = XMFLOAT3(camPos.X, camPos.Y, camPos.Z);
    camData.Pad0 = 0.0f;
    memcpy(fr.cameraCBMapped, &camData, sizeof(camData));

    // Light CB
    LightCBData lightData = {};
    lightData.DirLightDirection = XMFLOAT3(dirLight.Direction.X, dirLight.Direction.Y, dirLight.Direction.Z);
    lightData.DirLightIntensity = dirLight.Intensity;
    lightData.DirLightColor = XMFLOAT3(dirLight.Color.X, dirLight.Color.Y, dirLight.Color.Z);

    lightData.PointLightPosition = XMFLOAT3(pointLight.Position.X, pointLight.Position.Y, pointLight.Position.Z);
    lightData.PointLightIntensity = pointLight.Intensity;
    lightData.PointLightColor = XMFLOAT3(pointLight.Color.X, pointLight.Color.Y, pointLight.Color.Z);
    lightData.PointLightRange = pointLight.Range;
    lightData.PointLightEnabled = pointLight.Enabled ? 1 : 0;

    lightData.SpotLightPosition = XMFLOAT3(spotLight.Position.X, spotLight.Position.Y, spotLight.Position.Z);
    lightData.SpotLightIntensity = spotLight.Intensity;
    lightData.SpotLightDirection = XMFLOAT3(spotLight.Direction.X, spotLight.Direction.Y, spotLight.Direction.Z);
    lightData.SpotLightRange = spotLight.Range;
    lightData.SpotLightColor = XMFLOAT3(spotLight.Color.X, spotLight.Color.Y, spotLight.Color.Z);
    lightData.SpotLightInnerCos = cosf(Math::Deg2Rad(spotLight.InnerConeAngle));
    lightData.SpotLightOuterCos = cosf(Math::Deg2Rad(spotLight.OuterConeAngle));
    lightData.SpotLightEnabled = spotLight.Enabled ? 1 : 0;

    memcpy(fr.lightCBMapped, &lightData, sizeof(lightData));

    mCommandList->SetGraphicsRootConstantBufferView(1, fr.cameraCB->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootConstantBufferView(2, fr.lightCB->GetGPUVirtualAddress());

    mObjectDrawIndex = 0;
}

void D3D12Renderer::DrawMesh(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix)
{
    if (mObjectDrawIndex >= kMaxObjectsPerFrame)
    {
        return; // 이번 프레임 오브젝트 수 한도 초과 (필요하면 kMaxObjectsPerFrame 늘리기)
    }

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ObjectCBData objData;
    XMStoreFloat4x4(&objData.World, XMMatrixTranspose(ToXM(worldMatrix)));

    UINT8* dest = fr.objectCBMapped + mObjectDrawIndex * fr.objectCBStride;
    memcpy(dest, &objData, sizeof(objData));

    D3D12_GPU_VIRTUAL_ADDRESS objectCBAddress = fr.objectCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.objectCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(0, objectCBAddress);

    if (auto texture = material.GetTexture())
    {
        mCommandList->SetGraphicsRootDescriptorTable(3, texture->GetSrvHandle());
    }

    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    mCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

    mObjectDrawIndex++;
}

void D3D12Renderer::EndFrame()
{
    CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        mBackBuffers[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrierToPresent);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* commandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ThrowIfFailed(mSwapChain->Present(1, 0));

    mFenceValue++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
    mFrameResources[mCurrentFrameResourceIndex].fenceValue = mFenceValue;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % kFrameCount;
}

void D3D12Renderer::WaitForGpu()
{
    mFenceValue++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

    if (mFence->GetCompletedValue() < mFenceValue)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

void D3D12Renderer::Cleanup()
{
    WaitForGpu(); // 셧다운 시에는 완전 동기화로 안전하게 정리
    CloseHandle(mFenceEvent);
}

std::shared_ptr<Texture> D3D12Renderer::LoadTextureFromFile(const std::wstring& path)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    if (!AllocateSrvSlot(cpuHandle, gpuHandle)) return nullptr;

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];
    ThrowIfFailed(fr.commandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(fr.commandAllocator.Get(), nullptr));

    auto texture = std::make_shared<Texture>();
    texture->Create(mDevice.Get(), mCommandList.Get(), path, cpuHandle, gpuHandle);

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* lists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, lists);
    WaitForGpu();

    return texture;
}

std::shared_ptr<Texture> D3D12Renderer::CreateSolidColorTexture(BYTE r, BYTE g, BYTE b, BYTE a)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    if (!AllocateSrvSlot(cpuHandle, gpuHandle)) return nullptr;

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];
    ThrowIfFailed(fr.commandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(fr.commandAllocator.Get(), nullptr));

    auto texture = std::make_shared<Texture>();
    texture->CreateSolidColor(mDevice.Get(), mCommandList.Get(), r, g, b, a, cpuHandle, gpuHandle);

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* lists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, lists);
    WaitForGpu();

    return texture;
}