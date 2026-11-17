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

    //단일 CBuffer 객체(Camera, Light)들도 256바이트 정렬 할당 보장
    CreateUploadBuffer(device, Align256(sizeof(CameraCBData)), cameraCB, &cameraCBMapped);
    CreateUploadBuffer(device, Align256(sizeof(LightCBData)), lightCB, &lightCBMapped);

    // BoneCB:
    //   [0 .. maxObjectCount-1]                         : Main Pass (mObjectDrawIndex)
    //   [maxObjectCount + cascade*maxObjectCount .. ]   : Shadow Pass (cascade별 분리)
    // ShadowCB:
    //   [cascade*maxObjectCount .. ]                    : Cascade별 분리
    //
    // 이전에는 Main BoneCB와 Shadow BoneCB가 같은 주소 공간을 공유해서,
    // 한 프레임 안에서 Shadowdraw가 읽기 전에 Main 업로드가 값을 덮어쓰는
    // 문제가 있었다(스키닝 그림자 오염).
    const UINT shadowSlotCount = maxObjectCount * kNumCascades;

    boneCBStride = Align256(sizeof(BoneCBData));
    CreateUploadBuffer(device, boneCBStride * (maxObjectCount + shadowSlotCount), boneCB, &boneCBMapped);

    shadowCBStride = Align256(sizeof(ShadowCBData));
    CreateUploadBuffer(device, shadowCBStride * shadowSlotCount, shadowCB, &shadowCBMapped);
}