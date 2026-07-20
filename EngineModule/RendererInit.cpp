#include "pch.h"
#include "D3D12Renderer.h"

bool D3D12Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    mHwnd = hwnd;
    mWidth = width;
    mHeight = height;

    //3단계 그룹으로 쪼개 가독성 극대화
    InitializeCoreContext();
    InitializeRenderTargets();
    InitializeShadersAndPipelines();

    return true;
}

void D3D12Renderer::InitializeCoreContext()
{
    CreateFactory();
    CreateDevice();
    CreateCommandQueue();
    CreateFrameResources();
    CreateCommandList();
    CreateFence();
    CreateSwapChain();
}

void D3D12Renderer::InitializeRenderTargets()
{
    //개별적으로 분리한 DescriptorAllocator 객체들을 명확하게 초기화합니다.
    mSrvAllocator.Initialize(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSrvCount, true);
    mRtvAllocator.Initialize(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kFrameCount + 1, false);
    mDsvAllocator.Initialize(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, false);

    CreateRenderTargetViews();
    CreateDepthStencil();
    SetupViewport();
    InitializeViewport(1280, 720);
}

void D3D12Renderer::InitializeShadersAndPipelines()
{
    CreateShader();
    CreateRootSignature();
    CreatePSOs();
}

void D3D12Renderer::CreateFactory()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif
    ThrowIfFailed(CreateDXGIFactory2(_DEBUG ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&mFactory)));
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
    ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrameResources[0].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    mCommandList->Close();
}

void D3D12Renderer::CreateFence()
{
    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceValue = 0;
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr) ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
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

//기존 렌더 타겟 주소 검출 방식을 할당자 힙 주소로부터 정형화되게 추출
void D3D12Renderer::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < kFrameCount; ++i)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])));
        mDevice->CreateRenderTargetView(mBackBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, mRtvAllocator.GetDescriptorSize());
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
    ThrowIfFailed(mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&mDepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mDsvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart());
}

void D3D12Renderer::SetupViewport()
{
    mViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0f, 1.0f };
    mScissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
}

void D3D12Renderer::CompileShaderInternal(const std::wstring& filename, const char* entryPoint, const char* target, ID3DBlob** shaderBlob)
{
    UINT compileFlags = _DEBUG ? (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION) : 0;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(filename.c_str(), nullptr, nullptr, entryPoint, target, compileFlags, 0, shaderBlob, &errorBlob);
    if (FAILED(hr) && errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    ThrowIfFailed(hr);
}

void D3D12Renderer::CreateShader()
{
    CompileShaderInternal(L"Shaders/Default.hlsl", "VSMain", "vs_5_0", &mVertexShader);
    CompileShaderInternal(L"Shaders/Default.hlsl", "PSMain", "ps_5_0", &mPixelShader);
    CompileShaderInternal(L"Shaders/Default.hlsl", "VSMain_Skinned", "vs_5_0", &mSkinnedVertexShader);
}

void D3D12Renderer::CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParams[5];
    rootParams[0].InitAsConstantBufferView(0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);
    rootParams[3].InitAsConstantBufferView(3);
    rootParams[4].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

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

void D3D12Renderer::CreatePipelineState(ID3DBlob* vsBlob, ID3DBlob* psBlob, ID3D12PipelineState** outPSO)
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
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

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(outPSO)));
}

void D3D12Renderer::CreatePSOs()
{
    CreatePipelineState(mVertexShader.Get(), mPixelShader.Get(), &mPSO);
    CreatePipelineState(mSkinnedVertexShader.Get(), mPixelShader.Get(), &mSkinnedPSO);
}