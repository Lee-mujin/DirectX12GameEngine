#include "pch.h"
#include "FrameResource.h"

namespace
{
    UINT Align256(UINT size)
    {
        return (size + 255) & ~255;
    }

    void CreateUploadBuffer(ID3D12Device* device, UINT size, ComPtr<ID3D12Resource>& outResource, UINT8** outMapped)
    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&outResource)));

        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(outResource->Map(0, &readRange, reinterpret_cast<void**>(outMapped)));
    }
}

void FrameResource::Create(ID3D12Device* device, UINT maxObjectCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

    objectCBStride = Align256(sizeof(ObjectCBData));
    CreateUploadBuffer(device, objectCBStride * maxObjectCount, objectCB, &objectCBMapped);
    CreateUploadBuffer(device, Align256(sizeof(CameraCBData)), cameraCB, &cameraCBMapped);
    CreateUploadBuffer(device, Align256(sizeof(LightCBData)), lightCB, &lightCBMapped);
}