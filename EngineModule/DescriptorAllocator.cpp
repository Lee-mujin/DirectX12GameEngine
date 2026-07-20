#include "pch.h"
#include "DescriptorAllocator.h"

bool DescriptorAllocator::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount, bool shaderVisible)
{
    mMaxCount = maxCount;
    mDescriptorSize = device->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = mMaxCount;
    desc.Type = type;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap))))
    {
        return false;
    }

    mHeapStartCpu = mHeap->GetCPUDescriptorHandleForHeapStart();
    if (shaderVisible)
    {
        mHeapStartGpu = mHeap->GetGPUDescriptorHandleForHeapStart();
    }

    return true;
}

bool DescriptorAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu, D3D12_GPU_DESCRIPTOR_HANDLE& outGpu)
{
    UINT allocatedIndex = 0;

    //반환된 슬롯이 있다면 먼저 재사용
    if (!mFreeIndices.empty())
    {
        allocatedIndex = mFreeIndices.back();
        mFreeIndices.pop_back();
    }
    //없다면 순차적 오프셋 할당
    else
    {
        if (mUsedCount >= mMaxCount)
        {
            return false;
        }
        allocatedIndex = mUsedCount;
        mUsedCount++;
    }

    outCpu.ptr = mHeapStartCpu.ptr + (static_cast<SIZE_T>(allocatedIndex) * mDescriptorSize);
    if (mHeapStartGpu.ptr != 0)
    {
        outGpu.ptr = mHeapStartGpu.ptr + (allocatedIndex * mDescriptorSize);
    }
    else
    {
        outGpu.ptr = 0;
    }

    return true;
}

void DescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    //핸들 주소 연산을 통해 몇 번째 인덱스 슬롯인지 역산하여 FreeList에 삽입
    UINT index = GetIndexFromHandle(cpuHandle);
    mFreeIndices.push_back(index);
}

UINT DescriptorAllocator::GetIndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
    return static_cast<UINT>((cpuHandle.ptr - mHeapStartCpu.ptr) / mDescriptorSize);
}

void DescriptorAllocator::Shutdown()
{
    mHeap.Reset();
    mFreeIndices.clear();
    mUsedCount = 0;
}