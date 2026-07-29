#include "pch.h"
#include "D3D12Renderer.h"

bool D3D12Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
    mHwnd = hwnd;
    mWidth = width;
    mHeight = height;

    InitializeDevice();
    InitializeFrameResources();
    InitializeDescriptors();
    InitializeRenderTargets();
    InitializePipeline();

    return true;
}

//하드웨어 디바이스, 큐, 펜스 생성
void D3D12Renderer::InitializeDevice()
{
    CreateFactory();
    CreateDevice();
    CreateCommandQueue();
    CreateFence();
}

//프레임 리소스 버퍼 및 커맨드 리스트 초기화
void D3D12Renderer::InitializeFrameResources()
{
    CreateFrameResources();
    CreateCommandList();
}

//디스크립터 할당자 Heap 초기화
void D3D12Renderer::InitializeDescriptors()
{
    //Allocator 안전 초기화 (DSV Heap 개수를 8개로 늘려 슬롯 겹침 방지)
    mSrvAllocator.Initialize(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSrvCount, true);
    mRtvAllocator.Initialize(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kFrameCount + 1, false);
    mDsvAllocator.Initialize(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 8, false); //8개로 확장
}

//스왑체인, 백버퍼 RTV, 메인/섀도우 DSV 및 뷰포트 생성
void D3D12Renderer::InitializeRenderTargets()
{
    CreateSwapChain();
    CreateRenderTargetViews();
    CreateDepthStencil(); // 0번 슬롯: 메인 Depth Stencil View (D24_UNORM_S8_UINT)

    //Depth Stencil 생성 완료 후 1번 슬롯에 ShadowMap (D32_FLOAT) 생성
    InitializeShadowMap();

    SetupViewport();
    InitializeViewport(1280, 720);
}

//파이프라인(셰이더, 루트시그니처, PSO) 구축
void D3D12Renderer::InitializePipeline()
{
    CreateShader();
    CreateRootSignature();
    CreatePSOs();
}

bool D3D12Renderer::InitializeShadowMap()
{
    //ShadowMap 초기화 (Descriptor Heap 슬롯 안전 할당)
    return mShadowMap.Initialize(mDevice.Get(), &mDsvAllocator, &mSrvAllocator, 2048);
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

    //DSV 할당자의 Start 위치에 메인 DepthStencilView 생성
    DescriptorHandle mainDsvHandle = mDsvAllocator.Allocate();
    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mainDsvHandle.CpuHandle);
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

    CompileShaderInternal(L"Shaders/Shadow.hlsl", "VSMain", "vs_5_0", &mShadowVertexShader);
    CompileShaderInternal(L"Shaders/Shadow.hlsl", "VSMain_Skinned", "vs_5_0", &mShadowSkinnedVertexShader);
}

void D3D12Renderer::CreateRootSignature()
{
    // Main Pass Descriptor Ranges
    CD3DX12_DESCRIPTOR_RANGE srvRangeBase;
    srvRangeBase.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //BaseColor

    CD3DX12_DESCRIPTOR_RANGE srvRangeNormal;
    srvRangeNormal.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //NormalMap

    CD3DX12_DESCRIPTOR_RANGE srvRangeShadow;
    srvRangeShadow.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); //ShadowMap

    CD3DX12_ROOT_PARAMETER rootParams[7];
    rootParams[0].InitAsConstantBufferView(0); //ObjectCB
    rootParams[1].InitAsConstantBufferView(1); //CameraCB
    rootParams[2].InitAsConstantBufferView(2); //LightCB
    rootParams[3].InitAsConstantBufferView(3); //BoneCB
    rootParams[4].InitAsDescriptorTable(1, &srvRangeBase, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[5].InitAsDescriptorTable(1, &srvRangeNormal, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[6].InitAsDescriptorTable(1, &srvRangeShadow, D3D12_SHADER_VISIBILITY_PIXEL);

    //일반 텍스처용
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].ShaderRegister = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    //섀도우 Depth 비교 샘플러
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; //영역 밖은 그림자 안 생김
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].ShaderRegister = 1;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(_countof(rootParams), rootParams, _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signatureBlob, errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
    ThrowIfFailed(mDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

    CD3DX12_ROOT_PARAMETER shadowRootParams[2];
    shadowRootParams[0].InitAsConstantBufferView(0);
    shadowRootParams[1].InitAsConstantBufferView(1);

    CD3DX12_ROOT_SIGNATURE_DESC shadowRootSigDesc;
    shadowRootSigDesc.Init(_countof(shadowRootParams), shadowRootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> shadowSigBlob, shadowErrBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&shadowRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &shadowSigBlob, &shadowErrBlob));
    ThrowIfFailed(mDevice->CreateRootSignature(0, shadowSigBlob->GetBufferPointer(), shadowSigBlob->GetBufferSize(), IID_PPV_ARGS(&mShadowRootSignature)));
}

void D3D12Renderer::CreatePipelineState(ID3DBlob* vsBlob, ID3DBlob* psBlob, ID3D12RootSignature* rootSig, DXGI_FORMAT dsvFormat, bool isShadowPSO, ID3D12PipelineState** outPSO)
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 80, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;

    if (vsBlob) psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    if (psBlob) psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    if (isShadowPSO)
    {
        //가파른 경사면 정밀 보정을 위한 하드웨어 바이어스
        psoDesc.RasterizerState.DepthBias = 1000;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    }

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    if (isShadowPSO)
    {
        psoDesc.NumRenderTargets = 0;
    }
    else
    {
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    psoDesc.DSVFormat = dsvFormat;
    psoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(outPSO)));
}

void D3D12Renderer::CreatePSOs()
{
    //메인 패스 PSO (D24_UNORM_S8_UINT 사용)
    CreatePipelineState(mVertexShader.Get(), mPixelShader.Get(), mRootSignature.Get(), DXGI_FORMAT_D24_UNORM_S8_UINT, false, &mPSO);
    CreatePipelineState(mSkinnedVertexShader.Get(), mPixelShader.Get(), mRootSignature.Get(), DXGI_FORMAT_D24_UNORM_S8_UINT, false, &mSkinnedPSO);

    //섀도우 패스 PSO (D32_FLOAT 사용)
    CreatePipelineState(mShadowVertexShader.Get(), nullptr, mShadowRootSignature.Get(), DXGI_FORMAT_D32_FLOAT, true, &mShadowPSO);
    CreatePipelineState(mShadowSkinnedVertexShader.Get(), nullptr, mShadowRootSignature.Get(), DXGI_FORMAT_D32_FLOAT, true, &mShadowSkinnedPSO);
}