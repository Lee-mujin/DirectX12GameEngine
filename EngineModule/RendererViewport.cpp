#include "pch.h"
#include "D3D12Renderer.h"

void D3D12Renderer::InitializeViewport(UINT width, UINT height)
{
    //뷰포트용 핸들 주소 연산
    mViewportRtvHandle.ptr = mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart().ptr +
        (static_cast<SIZE_T>(kFrameCount) * mRtvAllocator.GetDescriptorSize());

    mViewportDsvHandle = mDsvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();

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

    mViewportTarget.Create(mDevice.Get(), width, height, mViewportRtvHandle, mViewportDsvHandle, mViewportSrvCpuHandle, mViewportSrvGpuHandle);
}

void D3D12Renderer::ResizeViewport(UINT width, UINT height)
{
    WaitForGpu();
    InitializeViewport(width, height);
}

void D3D12Renderer::TransitionToSwapChain()
{
    TransitionResource(mViewportTarget.GetColorResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    TransitionResource(mBackBuffers[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    //백버퍼 RTV 핸들
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRtvAllocator.GetDescriptorSize());
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    float editorBg[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, editorBg, 0, nullptr);
}