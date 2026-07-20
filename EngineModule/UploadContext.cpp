#include "pch.h"
#include "UploadContext.h"

bool UploadContext::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    mCommandQueue = queue;

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mAllocator)))) return false;
    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)))) return false;
    mCommandList->Close();

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)))) return false;
    mFenceValue = 0;
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return mFenceEvent != nullptr;
}

//Flush로 독립
void UploadContext::Flush()
{
    mFenceValue++;
    if (SUCCEEDED(mCommandQueue->Signal(mFence.Get(), mFenceValue)))
    {
        if (mFence->GetCompletedValue() < mFenceValue)
        {
            mFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
            WaitForSingleObject(mFenceEvent, INFINITE);
        }
    }
}

void UploadContext::Shutdown()
{
    mAllocator.Reset();
    mCommandList.Reset();
    mFence.Reset();
    if (mFenceEvent)
    {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
}