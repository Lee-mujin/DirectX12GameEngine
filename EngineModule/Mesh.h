#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <vector>
#include "Vertex.h"

using Microsoft::WRL::ComPtr;

class UploadContext;

class Mesh
{
public:
    Mesh() = default;
    virtual ~Mesh() = default;

    //UploadContext를 전달받아 Default Heap에 생성 및 Copy 명령 기록
    void Create(ID3D12Device* device, UploadContext* uploadContext, const std::vector<Vertex>& vertices, const std::vector<UINT16>& indices);

    //Graphics Command List에서 상태 전환
    void TransitionToRenderState(ID3D12GraphicsCommandList* cmdList);

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return mVertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return mIndexBufferView; }
    UINT GetIndexCount() const { return mIndexCount; }

private:
    ComPtr<ID3D12Resource> mVertexBuffer;
    ComPtr<ID3D12Resource> mIndexBuffer;

    //업로드 중 댕글링 방지용 임시 업로드 버퍼
    ComPtr<ID3D12Resource> mVertexUploadBuffer;
    ComPtr<ID3D12Resource> mIndexUploadBuffer;

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
    UINT mIndexCount = 0;

    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};