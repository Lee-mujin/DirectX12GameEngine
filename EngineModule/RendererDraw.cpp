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

    //Mesh 버퍼 상태 전환 (COMMON -> VERTEX/INDEX_BUFFER)
    const_cast<Mesh&>(mesh).TransitionToRenderState(mCommandList.Get());

    mCommandList->SetPipelineState(pso);

    //BaseColor 텍스처 처리 (Root Slot 4)
    std::shared_ptr<Texture> baseTexture = material.GetTexture();
    if (!baseTexture)
    {
        baseTexture = mDefaultWhiteTexture;
    }

    if (baseTexture)
    {
        const_cast<Texture&>(*baseTexture).TransitionToRenderState(mCommandList.Get());
        mCommandList->SetGraphicsRootDescriptorTable(4, baseTexture->GetSrvHandle());
    }

    //NormalMap 텍스처 처리 (Root Slot 5)
    std::shared_ptr<Texture> normalTexture = material.GetNormalTexture();
    bool hasNormalMap = (normalTexture != nullptr);

    if (hasNormalMap)
    {
        const_cast<Texture&>(*normalTexture).TransitionToRenderState(mCommandList.Get());
        mCommandList->SetGraphicsRootDescriptorTable(5, normalTexture->GetSrvHandle());
    }
    else
    {
        // 바인딩 안 하면 이전 드로우의 값이 남을 수 있어 기본 텍스처로 대치 바인딩
        if (mDefaultWhiteTexture)
        {
            mCommandList->SetGraphicsRootDescriptorTable(5, mDefaultWhiteTexture->GetSrvHandle());
        }
    }

    // 4. Constant Buffer 데이터 채우기
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ObjectCBData objData;
    XMStoreFloat4x4(&objData.World, XMMatrixTranspose(D3D12RendererToXM(worldMatrix)));

    Vector3 matColor = material.GetBaseColor();
    objData.MaterialColor = XMFLOAT3(matColor.X, matColor.Y, matColor.Z);
    objData.Shininess = material.GetShininess();

    //5. 노멀 맵 적용 유무 플래그 전달
    objData.HasNormalMap = hasNormalMap ? 1 : 0;

    UINT8* objDest = fr.objectCBMapped + mObjectDrawIndex * fr.objectCBStride;
    memcpy(objDest, &objData, sizeof(objData));

    D3D12_GPU_VIRTUAL_ADDRESS objectCBAddress = fr.objectCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.objectCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(0, objectCBAddress);

    // 6. Skinned Animation Bone Constant Buffer
    if (boneData)
    {
        UINT8* boneDest = fr.boneCBMapped + mObjectDrawIndex * fr.boneCBStride;
        memcpy(boneDest, boneData, sizeof(BoneCBData));

        D3D12_GPU_VIRTUAL_ADDRESS boneCBAddress = fr.boneCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.boneCBStride;
        mCommandList->SetGraphicsRootConstantBufferView(3, boneCBAddress);
    }

    // 7. Draw Call 수행
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