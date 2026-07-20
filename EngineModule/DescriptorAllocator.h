#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class DescriptorAllocator
{
public:
    DescriptorAllocator() = default;
    ~DescriptorAllocator() = default;

    // 디스크립터 힙 초기화
    bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount, bool shaderVisible);
    void Shutdown();

    // 슬롯 하나 할당 받기
    bool Allocate(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu, D3D12_GPU_DESCRIPTOR_HANDLE& outGpu);

    // 사용이 끝난 슬롯 반환 (에셋 해제용 FreeList)
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    ID3D12DescriptorHeap* GetHeap() const { return mHeap.Get(); }
    UINT GetDescriptorSize() const { return mDescriptorSize; }
    UINT GetUsedCount() const { return mUsedCount; }

private:
    UINT GetIndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mHeapStartCpu{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE mHeapStartGpu{ 0 };

    UINT mDescriptorSize = 0;
    UINT mMaxCount = 0;
    UINT mUsedCount = 0;

    //반환된 슬롯 인덱스를 보관하여 재사용하는 FreeList 구조
    std::vector<UINT> mFreeIndices;
};