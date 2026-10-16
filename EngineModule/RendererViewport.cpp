#include "pch.h"
#include "D3D12Renderer.h"

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Renderer::GetViewportSrvHandle() const
{
    switch (mDisplayMode)
    {
    case ViewportDisplayMode::BrightPass:
    case ViewportDisplayMode::BloomBlur:
        if (mBloomTargetA.GetSrvHandle().ptr != 0) return mBloomTargetA.GetSrvHandle();
        break;
    case ViewportDisplayMode::HDRRaw:
        if (mHDRRenderTarget.GetSrvHandle().ptr != 0) return mHDRRenderTarget.GetSrvHandle();
        break;
    default:
        break;
    }
    return mViewportTarget.GetSrvHandle();
}

void D3D12Renderer::InitializeViewport(UINT width, UINT height)
{
    //뷰포트용 핸들 주소 연산 (kFrameCount 슬롯: 2번)
    mViewportRtvHandle.ptr = mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart().ptr +
        (static_cast<SIZE_T>(kFrameCount) * mRtvAllocator.GetDescriptorSize());

    if (!mIsViewportSrvAllocated)
    {
        //단일 DescriptorHandle 객체로 할당
        DescriptorHandle srvHandle = mSrvAllocator.Allocate();
        if (srvHandle)
        {
            mViewportSrvCpuHandle = srvHandle.CpuHandle;
            mViewportSrvGpuHandle = srvHandle.GpuHandle;
            mIsViewportSrvAllocated = true;
        }
    }

    if (!mIsViewportSrvAllocated) return;

    D3D12_CPU_DESCRIPTOR_HANDLE dummyDsv = { 0 };
    mViewportTarget.Create(mDevice.Get(), width, height, mViewportRtvHandle, dummyDsv, mViewportSrvCpuHandle, mViewportSrvGpuHandle, mViewportFormat, false);
}

void D3D12Renderer::InitializeHDRRenderTarget(UINT width, UINT height)
{
    //HDR 렌더 타깃용 RTV 핸들 주소 연산 (kFrameCount + 1 슬롯: 3번)
    mHDRRtvHandle.ptr = mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart().ptr +
        (static_cast<SIZE_T>(kFrameCount + 1) * mRtvAllocator.GetDescriptorSize());

    mHDRDsvHandle = mDsvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();

    if (!mIsHDRSrvAllocated)
    {
        DescriptorHandle srvHandle = mSrvAllocator.Allocate();
        if (srvHandle)
        {
            mHDRSrvCpuHandle = srvHandle.CpuHandle;
            mHDRSrvGpuHandle = srvHandle.GpuHandle;
            mIsHDRSrvAllocated = true;
        }
    }

    if (!mIsHDRSrvAllocated) return;

    mHDRRenderTarget.SetClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
    mHDRRenderTarget.Create(mDevice.Get(), width, height, mHDRRtvHandle, mHDRDsvHandle, mHDRSrvCpuHandle, mHDRSrvGpuHandle, mHDRFormat);
}

void D3D12Renderer::InitializeBloomRenderTargets(UINT width, UINT height)
{
    UINT bloomWidth = max(width / 2, 1u);
    UINT bloomHeight = max(height / 2, 1u);

    //블룸 핑퐁용 RTV 슬롯 (kFrameCount + 2: 4번, kFrameCount + 3: 5번)
    mBloomARtvHandle.ptr = mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart().ptr +
        (static_cast<SIZE_T>(kFrameCount + 2) * mRtvAllocator.GetDescriptorSize());

    mBloomBRtvHandle.ptr = mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart().ptr +
        (static_cast<SIZE_T>(kFrameCount + 3) * mRtvAllocator.GetDescriptorSize());

    if (!mIsBloomASrvAllocated)
    {
        DescriptorHandle srvHandle = mSrvAllocator.Allocate();
        if (srvHandle)
        {
            mBloomASrvCpuHandle = srvHandle.CpuHandle;
            mBloomASrvGpuHandle = srvHandle.GpuHandle;
            mIsBloomASrvAllocated = true;
        }
    }

    if (!mIsBloomBSrvAllocated)
    {
        DescriptorHandle srvHandle = mSrvAllocator.Allocate();
        if (srvHandle)
        {
            mBloomBSrvCpuHandle = srvHandle.CpuHandle;
            mBloomBSrvGpuHandle = srvHandle.GpuHandle;
            mIsBloomBSrvAllocated = true;
        }
    }

    if (!mIsBloomASrvAllocated || !mIsBloomBSrvAllocated) return;

    D3D12_CPU_DESCRIPTOR_HANDLE dummyDsv = { 0 };
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    mBloomTargetA.SetClearColor(black[0], black[1], black[2], black[3]);
    mBloomTargetA.Create(mDevice.Get(), bloomWidth, bloomHeight, mBloomARtvHandle, dummyDsv, mBloomASrvCpuHandle, mBloomASrvGpuHandle, mHDRFormat, false);

    mBloomTargetB.SetClearColor(black[0], black[1], black[2], black[3]);
    mBloomTargetB.Create(mDevice.Get(), bloomWidth, bloomHeight, mBloomBRtvHandle, dummyDsv, mBloomBSrvCpuHandle, mBloomBSrvGpuHandle, mHDRFormat, false);
}

void D3D12Renderer::ResizeViewport(UINT width, UINT height)
{
    WaitForGpu();
    InitializeViewport(width, height);
    InitializeHDRRenderTarget(width, height);
    InitializeBloomRenderTargets(width, height);
}

void D3D12Renderer::ExecuteBrightPass()
{
    if (!mHDRRenderTarget.GetColorResource() || !mBloomTargetA.GetColorResource()) return;

    // 1. HDR 타깃: RENDER_TARGET -> PIXEL_SHADER_RESOURCE (셰이더에서 읽을 준비)
    TransitionResource(mHDRRenderTarget.GetColorResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // 2. Bloom 타깃 A: PIXEL_SHADER_RESOURCE -> RENDER_TARGET (고광도 픽셀 기록 준비)
    TransitionResource(mBloomTargetA.GetColorResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 3. 1/2 다운샘플링 해상도로 BrightPass 실행
    mBrightPass.Execute(
        mCommandList.Get(),
        mHDRRenderTarget.GetSrvHandle(),
        mBloomTargetA.GetRtvHandle(),
        mBloomTargetA.GetWidth(),
        mBloomTargetA.GetHeight(),
        mBrightPassParams
    );

    // 4. Bloom 타깃 A: RENDER_TARGET -> PIXEL_SHADER_RESOURCE (추후 블러 또는 디버그 뷰에서 읽을 준비)
    TransitionResource(mBloomTargetA.GetColorResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void D3D12Renderer::ExecuteGaussianBlur()
{
    if (!mBloomTargetA.GetColorResource() || !mBloomTargetB.GetColorResource()) return;

    // 사용자가 BrightPass 원본 화면만 모니터링 중인 경우, 블러를 건너뛰어 원본 BrightPass가 유지되도록 함
    if (mDisplayMode == ViewportDisplayMode::BrightPass) return;

    UINT width = mBloomTargetA.GetWidth();
    UINT height = mBloomTargetA.GetHeight();

    for (int iter = 0; iter < mBlurIterations; ++iter)
    {
        // 1. 수평 블러 (Horizontal Pass): BloomTargetA (SRV) -> BloomTargetB (RTV)
        TransitionResource(mBloomTargetB.GetColorResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        mGaussianBlurPass.Execute(
            mCommandList.Get(),
            mBloomTargetA.GetSrvHandle(),
            mBloomTargetB.GetRtvHandle(),
            width, height,
            1.0f, 0.0f,
            mBlurSigma
        );

        TransitionResource(mBloomTargetB.GetColorResource(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // 2. 수직 블러 (Vertical Pass): BloomTargetB (SRV) -> BloomTargetA (RTV)
        TransitionResource(mBloomTargetA.GetColorResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        mGaussianBlurPass.Execute(
            mCommandList.Get(),
            mBloomTargetB.GetSrvHandle(),
            mBloomTargetA.GetRtvHandle(),
            width, height,
            0.0f, 1.0f,
            mBlurSigma
        );

        TransitionResource(mBloomTargetA.GetColorResource(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

void D3D12Renderer::ExecuteToneMappingPass()
{
    if (!mHDRRenderTarget.GetColorResource() || !mViewportTarget.GetColorResource()) return;

    // 1. LDR 뷰포트 타깃: PIXEL_SHADER_RESOURCE -> RENDER_TARGET (톤매핑 결과 기록 준비)
    TransitionResource(mViewportTarget.GetColorResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 2. 풀스크린 트라이앵글 톤매핑 패스 실행 (HDR SRV + Bloom SRV -> LDR RTV)
    D3D12_GPU_DESCRIPTOR_HANDLE bloomSrv = mBloomTargetA.GetColorResource() ? mBloomTargetA.GetSrvHandle() : mHDRRenderTarget.GetSrvHandle();

    mToneMappingPass.Execute(
        mCommandList.Get(),
        mHDRRenderTarget.GetSrvHandle(),
        bloomSrv,
        mViewportTarget.GetRtvHandle(),
        mViewportTarget.GetWidth(),
        mViewportTarget.GetHeight(),
        mToneMappingParams
    );

    // 3. LDR 뷰포트 타깃: RENDER_TARGET -> PIXEL_SHADER_RESOURCE (ImGui 뷰포트 창에서 읽을 준비)
    TransitionResource(mViewportTarget.GetColorResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void D3D12Renderer::TransitionToSwapChain()
{
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    TransitionResource(mBackBuffers[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    //백버퍼 RTV 핸들
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRtvAllocator.GetDescriptorSize());
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    float editorBg[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, editorBg, 0, nullptr);
}