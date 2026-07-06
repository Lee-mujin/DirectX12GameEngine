#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <vector>
#include "Vertex.h"

using Microsoft::WRL::ComPtr;

class Mesh
{
public:
    void Create(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<UINT16>& indices);

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return mVertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return mIndexBufferView; }
    UINT GetIndexCount() const { return mIndexCount; }

private:
    ComPtr<ID3D12Resource> mVertexBuffer;
    ComPtr<ID3D12Resource> mIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
    UINT mIndexCount = 0;
};