#include "pch.h"
#include "DescriptorAllocator.h"

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