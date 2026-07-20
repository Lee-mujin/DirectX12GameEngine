#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class UploadContext
{
public:
    UploadContext() = default;
    ~UploadContext() = default;

    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue);
    void Shutdown();

    //uploadFunc를 기록하고 명령을 큐에 제출, GPU를 안기다림
    template<typename Func>
    void Execute(Func&& uploadFunc)
    {
        mAllocator->Reset();
        mCommandList->Reset(mAllocator.Get(), nullptr);

        uploadFunc(mCommandList.Get());

        mCommandList->Close();
        ID3D12CommandList* lists[] = { mCommandList.Get() };
        mCommandQueue->ExecuteCommandLists(1, lists);
    }

    //누적된 대량의 GPU 복사 작업이 끝난 후, 단 한 번만 명시적으로 대기할 때 호출
    void Flush();

private:
    ComPtr<ID3D12CommandAllocator> mAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12Fence> mFence;
    HANDLE mFenceEvent = nullptr;
    UINT64 mFenceValue = 0;
    ID3D12CommandQueue* mCommandQueue = nullptr;
};