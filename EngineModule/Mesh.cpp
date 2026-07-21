#include "pch.h"
#include "Mesh.h"
#include "UploadContext.h"
#include "d3dx12.h"

void Mesh::Create(ID3D12Device* device, UploadContext* uploadContext, const std::vector<Vertex>& vertices, const std::vector<UINT16>& indices)
{
    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    mIndexCount = static_cast<UINT>(indices.size());
    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(UINT16));

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    //COPY 큐와 Graphics 큐 공유를 위한 COMMON 상태 생성
    mState = D3D12_RESOURCE_STATE_COMMON;

    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
        mState, nullptr, IID_PPV_ARGS(&mVertexBuffer)));

    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
        mState, nullptr, IID_PPV_ARGS(&mIndexBuffer)));

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexUploadBuffer)));

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIndexUploadBuffer)));

    UINT8* mappedVertex = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(mVertexUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedVertex)));
    memcpy(mappedVertex, vertices.data(), vertexBufferSize);
    mVertexUploadBuffer->Unmap(0, nullptr);

    UINT8* mappedIndex = nullptr;
    ThrowIfFailed(mIndexUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedIndex)));
    memcpy(mappedIndex, indices.data(), indexBufferSize);
    mIndexUploadBuffer->Unmap(0, nullptr);

    // Copy Queue에서는 복사만 수행
    uploadContext->Execute([this, vertexBufferSize, indexBufferSize](ID3D12GraphicsCommandList* cmdList) {
        cmdList->CopyBufferRegion(mVertexBuffer.Get(), 0, mVertexUploadBuffer.Get(), 0, vertexBufferSize);
        cmdList->CopyBufferRegion(mIndexBuffer.Get(), 0, mIndexUploadBuffer.Get(), 0, indexBufferSize);
        });

    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
    mVertexBufferView.SizeInBytes = vertexBufferSize;

    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    mIndexBufferView.SizeInBytes = indexBufferSize;
}

void Mesh::TransitionToRenderState(ID3D12GraphicsCommandList* cmdList)
{
    if (mState == D3D12_RESOURCE_STATE_COMMON)
    {
        CD3DX12_RESOURCE_BARRIER barriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mVertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
            CD3DX12_RESOURCE_BARRIER::Transition(mIndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER)
        };
        cmdList->ResourceBarrier(2, barriers);
        mState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
}