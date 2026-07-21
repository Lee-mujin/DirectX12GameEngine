#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class UploadContext
{
public:
    UploadContext() = default;
    ~UploadContext() = default;

    bool Initialize(ID3D12Device* device);
    void Shutdown();

    template<typename Func>
    void Execute(Func&& uploadFunc)
    {
        mAllocator->Reset();
        mCommandList->Reset(mAllocator.Get(), nullptr);

        uploadFunc(mCommandList.Get());

        mCommandList->Close();
        ID3D12CommandList* lists[] = { mCommandList.Get() };
        mCopyQueue->ExecuteCommandLists(1, lists);
    }

    // CPU에서 GPU 작업 완료까지 블로킹 대기
    void Flush();

    // Graphics Queue가 Copy Queue의 완료를 GPU 상에서 대기하도록 지시
    void SyncWithGraphicsQueue(ID3D12CommandQueue* graphicsQueue);

private:
    ComPtr<ID3D12CommandQueue> mCopyQueue; //전용 Copy Queue
    ComPtr<ID3D12CommandAllocator> mAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12Fence> mFence;
    HANDLE mFenceEvent = nullptr;
    UINT64 mFenceValue = 0;
};