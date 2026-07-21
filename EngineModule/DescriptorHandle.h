#pragma once
#include <d3d12.h>

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle{ 0 };
    UINT Index = UINT_MAX;

    bool IsValid() const { return CpuHandle.ptr != 0; }
    operator bool() const { return IsValid(); }
    void Reset()
    {
        CpuHandle.ptr = 0;
        GpuHandle.ptr = 0;
        Index = UINT_MAX;
    }
};