#include "pch.h"
#include "D3D12Renderer.h"
#include "MathUtil.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "SkinnedMesh.h"

using namespace DirectX;

extern XMMATRIX D3D12RendererToXM(const Matrix4x4& m);

void D3D12Renderer::DrawMeshInternal(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const BoneCBData* boneData, ID3D12PipelineState* pso)
{
    if (mObjectDrawIndex >= kMaxObjectsPerFrame) return;

    mCommandList->SetPipelineState(pso);

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ObjectCBData objData;
    XMStoreFloat4x4(&objData.World, XMMatrixTranspose(D3D12RendererToXM(worldMatrix)));
    Vector3 matColor = material.GetColor();
    objData.MaterialColor = XMFLOAT3(matColor.X, matColor.Y, matColor.Z);
    objData.Shininess = material.GetShininess();

    UINT8* objDest = fr.objectCBMapped + mObjectDrawIndex * fr.objectCBStride;
    memcpy(objDest, &objData, sizeof(objData));

    D3D12_GPU_VIRTUAL_ADDRESS objectCBAddress = fr.objectCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.objectCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(0, objectCBAddress);

    if (boneData)
    {
        UINT8* boneDest = fr.boneCBMapped + mObjectDrawIndex * fr.boneCBStride;
        memcpy(boneDest, boneData, sizeof(BoneCBData));

        D3D12_GPU_VIRTUAL_ADDRESS boneCBAddress = fr.boneCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.boneCBStride;
        mCommandList->SetGraphicsRootConstantBufferView(3, boneCBAddress);
    }

    if (auto texture = material.GetTexture())
    {
        mCommandList->SetGraphicsRootDescriptorTable(4, texture->GetSrvHandle());
    }

    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    mCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

    mObjectDrawIndex++;
}

void D3D12Renderer::DrawMesh(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix)
{
    DrawMeshInternal(mesh, material, worldMatrix, nullptr, mPSO.Get());
}

void D3D12Renderer::DrawSkinnedMesh(const SkinnedMesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const std::vector<Matrix4x4>& boneMatrices)
{
    BoneCBData boneData;
    UINT boneCount = static_cast<UINT>(boneMatrices.size());
    for (UINT i = 0; i < MAX_BONES; ++i)
    {
        if (i < boneCount)
            XMStoreFloat4x4(&boneData.BoneMatrices[i], XMMatrixTranspose(D3D12RendererToXM(boneMatrices[i])));
        else
            XMStoreFloat4x4(&boneData.BoneMatrices[i], XMMatrixIdentity());
    }

    DrawMeshInternal(mesh, material, worldMatrix, &boneData, mSkinnedPSO.Get());
}