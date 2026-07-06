#include "pch.h"

void Mesh::Create(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<UINT16>& indices)
{
    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

    CD3DX12_HEAP_PROPERTIES vbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &vbHeapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&mVertexBuffer)));

    UINT8* mappedVertex = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedVertex)));
    memcpy(mappedVertex, vertices.data(), vertexBufferSize);
    mVertexBuffer->Unmap(0, nullptr);

    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
    mVertexBufferView.SizeInBytes = vertexBufferSize;

    mIndexCount = static_cast<UINT>(indices.size());
    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(UINT16));

    CD3DX12_HEAP_PROPERTIES ibHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &ibHeapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&mIndexBuffer)));

    UINT8* mappedIndex = nullptr;
    ThrowIfFailed(mIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedIndex)));
    memcpy(mappedIndex, indices.data(), indexBufferSize);
    mIndexBuffer->Unmap(0, nullptr);

    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    mIndexBufferView.SizeInBytes = indexBufferSize;
}