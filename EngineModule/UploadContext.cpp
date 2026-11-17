#include "pch.h"
#include "UploadContext.h"

bool UploadContext::Initialize(ID3D12Device* device)
{
    //D3D12_COMMAND_LIST_TYPE_COPY 전용 Command Queue 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyQueue)))) return false;

    //COPY 타입 할당자 및 커맨드 리스트 생성
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mAllocator)))) return false;
    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)))) return false;
    mCommandList->Close();

    //Fence 및 Event 생성
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)))) return false;
    mFenceValue = 0;
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return mFenceEvent != nullptr;
}

void UploadContext::ProcessCompletedReleases()
{
    if (!mFence) return;

    const UINT64 completed = mFence->GetCompletedValue();
    for (auto it = mPendingReleases.begin(); it != mPendingReleases.end(); )
    {
        if (completed >= it->FenceValue)
        {
            it = mPendingReleases.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void UploadContext::ReleaseWhenComplete(ComPtr<ID3D12Resource> resource)
{
    if (!resource) return;

    PendingRelease pending;
    pending.FenceValue = mFenceValue;
    pending.Resource = std::move(resource);
    mPendingReleases.push_back(std::move(pending));
}

void UploadContext::SyncWithGraphicsQueue(ID3D12CommandQueue* graphicsQueue)
{
    // 업로드가 없으면 GPU Wait를 걸지 않는다 (불필요한 큐 직렬화 방지).
    if (!graphicsQueue || !mCopyQueue || !mFence || !mHasPendingUploads) return;

    mFenceValue++;
    //Copy Queue에 Signal 신호 적재
    if (FAILED(mCopyQueue->Signal(mFence.Get(), mFenceValue))) return;
    //Graphics Queue가 CPU 대기 없이 해당 Signal을 GPU 상에서 Wait 하도록 전달
    graphicsQueue->Wait(mFence.Get(), mFenceValue);
    mHasPendingUploads = false;
}

void UploadContext::Flush()
{
    if (!mCopyQueue || !mFence) return;

    mFenceValue++;
    if (SUCCEEDED(mCopyQueue->Signal(mFence.Get(), mFenceValue)))
    {
        if (mFence->GetCompletedValue() < mFenceValue)
        {
            mFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
            WaitForSingleObject(mFenceEvent, INFINITE);
        }
        mHasPendingUploads = false;
    }
    ProcessCompletedReleases();
}

void UploadContext::Shutdown()
{
    Flush();
    mPendingReleases.clear();

    mCopyQueue.Reset();
    mAllocator.Reset();
    mCommandList.Reset();
    mFence.Reset();
    if (mFenceEvent)
    {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
}
