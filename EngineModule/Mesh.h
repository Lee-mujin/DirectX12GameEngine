#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <vector>
#include "Vertex.h"
#include "BoundingBox.h"

using Microsoft::WRL::ComPtr;

class UploadContext;

struct SubMesh
{
    UINT IndexStart = 0;          // StartIndexLocation
    UINT IndexCount = 0;          // IndexCountPerInstance
    INT BaseVertexLocation = 0;   // BaseVertexLocation
    UINT MaterialIndex = 0;       // Model 머티리얼 인덱스
    BoundingBox Bounds;           // 서브메시 단위 바운딩 박스
};

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

    //서브메시 접근자
    const std::vector<SubMesh>& GetSubMeshes() const { return mSubMeshes; }
    void SetSubMeshes(std::vector<SubMesh> subMeshes) { mSubMeshes = std::move(subMeshes); }

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
    std::vector<SubMesh> mSubMeshes;
};