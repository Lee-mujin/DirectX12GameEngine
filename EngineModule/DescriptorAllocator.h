#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include "DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

class DescriptorAllocator
{
public:
    DescriptorAllocator() = default;
    ~DescriptorAllocator() = default;

    bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, bool shaderVisible = false);

    //기존ㅇ 단일 DescriptorHandle 반환 구조로 변경
    DescriptorHandle Allocate();
    void Free(DescriptorHandle& handle);

    ID3D12DescriptorHeap* GetHeap() const { return mHeap.Get(); }
    UINT GetCapacity() const { return mCapacity; }
    UINT GetDescriptorSize() const { return mDescriptorSize; }

private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mCpuBaseHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE mGpuBaseHandle{ 0 };

    UINT mDescriptorSize = 0;
    UINT mCapacity = 0;

    std::vector<bool> mAllocatedSlots;
    bool mShaderVisible = false;
};