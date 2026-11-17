#include "pch.h"
#include "DescriptorAllocator.h"

//비트맵 할당자
bool DescriptorAllocator::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, bool shaderVisible)
{
    mCapacity = capacity;
    mShaderVisible = shaderVisible;
    mAllocatedSlots.assign(capacity, false);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = capacity;
    desc.Type = type;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)))) return false;

    mDescriptorSize = device->GetDescriptorHandleIncrementSize(type);
    mCpuBaseHandle = mHeap->GetCPUDescriptorHandleForHeapStart();

    if (mShaderVisible)
    {
        mGpuBaseHandle = mHeap->GetGPUDescriptorHandleForHeapStart();
    }

    return true;
}

DescriptorHandle DescriptorAllocator::Allocate()
{
    DescriptorHandle handle;

    for (UINT i = 0; i < mCapacity; ++i)
    {
        if (!mAllocatedSlots[i])
        {
            mAllocatedSlots[i] = true;

            handle.Index = i;
            handle.CpuHandle.ptr = mCpuBaseHandle.ptr + static_cast<SIZE_T>(i) * mDescriptorSize;

            if (mShaderVisible)
            {
                handle.GpuHandle.ptr = mGpuBaseHandle.ptr + static_cast<UINT64>(i) * mDescriptorSize;
            }
            return handle;
        }
    }

    // 힙 고갈은 조용히 실패하면 텍스처가 그리드에서 사라진다. 반드시 로그를 남긴다.
    OutputDebugStringA("[DescriptorAllocator] Heap exhausted - Allocate() returned an invalid handle.\n");
    return handle;
}

void DescriptorAllocator::Free(DescriptorHandle& handle)
{
    if (handle.Index < mCapacity)
    {
        mAllocatedSlots[handle.Index] = false;
        handle.Reset();
    }
}