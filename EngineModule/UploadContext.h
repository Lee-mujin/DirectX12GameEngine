#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

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
        // 이전 업로드 작업이 완료될 때까지 대기 (같은 커맨드 리스트/할당자 재사용)
        if (mFence && mFenceEvent)
        {
            if (mFence->GetCompletedValue() < mFenceValue)
            {
                mFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
                WaitForSingleObject(mFenceEvent, INFINITE);
            }
        }

        // 완료된 업로드 버퍼 해제
        ProcessCompletedReleases();

        //GPU 작업 완료 후 안전하게 Reset
        mAllocator->Reset();
        mCommandList->Reset(mAllocator.Get(), nullptr);

        //자원 복사 명령 기록
        uploadFunc(mCommandList.Get());

        //커맨드 리스트 닫고 CopyQueue 제출
        mCommandList->Close();
        ID3D12CommandList* lists[] = { mCommandList.Get() };
        mCopyQueue->ExecuteCommandLists(1, lists);

        //완료 판정을 위한 Fence Signal 발행
        mFenceValue++;
        mCopyQueue->Signal(mFence.Get(), mFenceValue);
        mHasPendingUploads = true;
    }

    // CPU에서 GPU 작업 완료까지 블로킹 대기
    void Flush();

    // Graphics Queue가 Copy Queue의 완료를 GPU 상에서 대기하도록 지시.
    // 업로드가 없는 프레임에는 아무 작업도 하지 않는다.
    void SyncWithGraphicsQueue(ID3D12CommandQueue* graphicsQueue);

    // Copy 완료 후 안전하게 해제할 리소스 등록 (Mesh/Texture 업로드 버퍼 반환용)
    void ReleaseWhenComplete(ComPtr<ID3D12Resource> resource);

private:
    void ProcessCompletedReleases();

    struct PendingRelease
    {
        UINT64 FenceValue = 0;
        ComPtr<ID3D12Resource> Resource;
    };

    ComPtr<ID3D12CommandQueue> mCopyQueue; //전용 Copy Queue
    ComPtr<ID3D12CommandAllocator> mAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12Fence> mFence;
    HANDLE mFenceEvent = nullptr;
    UINT64 mFenceValue = 0;
    bool mHasPendingUploads = false;

    std::vector<PendingRelease> mPendingReleases;
};
