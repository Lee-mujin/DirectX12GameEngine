#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <vector>
#include "Vertex.h"
#include "BoundingBox.h"

using Microsoft::WRL::ComPtr;

class UploadContext;

class Mesh
{
public:
    Mesh() = default;
    virtual ~Mesh() = default;

    //UINT32 인덱스 버퍼 지원 (FBX 모델 대응)
    void Create(ID3D12Device* device, UploadContext* uploadContext, const std::vector<Vertex>& vertices, const std::vector<UINT32>& indices);

    void TransitionToRenderState(ID3D12GraphicsCommandList* cmdList);

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return mVertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return mIndexBufferView; }
    UINT GetIndexCount() const { return mIndexCount; }

    const BoundingBox& GetBoundingBox() const { return mBoundingBox; }
    void SetBoundingBox(const BoundingBox& box) { mBoundingBox = box; }

private:
    ComPtr<ID3D12Resource> mVertexBuffer;
    ComPtr<ID3D12Resource> mIndexBuffer;

    ComPtr<ID3D12Resource> mVertexUploadBuffer;
    ComPtr<ID3D12Resource> mIndexUploadBuffer;

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
    UINT mIndexCount = 0;

    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;

    BoundingBox mBoundingBox;
};