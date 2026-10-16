#include "pch.h"
#include "D3D12Renderer.h"
#include "MathUtil.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "SkinnedMesh.h"
#include "DirectionalLight.h"
#include "CSMCalculator.h"

using namespace DirectX;

extern XMMATRIX D3D12RendererToXM(const Matrix4x4& m);

//메시 상태 전이 및 인덱스 기반 DrawCall 수행
void D3D12Renderer::DrawGeometry(const Mesh& mesh, UINT indexCount, UINT indexStart, INT baseVertexLocation)
{
    const_cast<Mesh&>(mesh).TransitionToRenderState(mCommandList.Get());
    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    UINT count = (indexCount > 0) ? indexCount : mesh.GetIndexCount();
    mCommandList->DrawIndexedInstanced(count, 1, indexStart, baseVertexLocation, 0);
}

void D3D12Renderer::BindItemTextures(const RenderItem& item)
{
    if (item.BaseTextureHandle.ptr != 0)
    {
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::BaseTexture), item.BaseTextureHandle);
    }
    else if (mDefaultWhiteTexture)
    {
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::BaseTexture), mDefaultWhiteTexture->GetSrvHandle());
    }

    if (item.HasNormalMap && item.NormalTextureHandle.ptr != 0)
    {
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::NormalTexture), item.NormalTextureHandle);
    }
    else if (mDefaultWhiteTexture)
    {
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::NormalTexture), mDefaultWhiteTexture->GetSrvHandle());
    }

    mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::ShadowTexture), mShadowMap.GetSRV());
}

void D3D12Renderer::UploadObjectCB(const RenderItem& item)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ObjectCBData objData;
    XMStoreFloat4x4(&objData.World, XMMatrixTranspose(D3D12RendererToXM(item.WorldMatrix)));

    objData.MaterialColor = XMFLOAT3(item.BaseColor.X, item.BaseColor.Y, item.BaseColor.Z);
    objData.Shininess = item.Shininess;
    objData.Metallic = item.Metallic;
    objData.Roughness = item.Roughness;
    objData.HasNormalMap = item.HasNormalMap ? 1 : 0;
    objData.Pad0 = 0.0f;

    UINT8* objDest = fr.objectCBMapped + mObjectDrawIndex * fr.objectCBStride;
    memcpy(objDest, &objData, sizeof(objData));

    D3D12_GPU_VIRTUAL_ADDRESS objectCBAddress = fr.objectCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.objectCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(RootSlot::ObjectCB), objectCBAddress);
}

void D3D12Renderer::UploadBoneCB(const std::vector<Matrix4x4>* boneMatrices)
{
    if (!boneMatrices) return;

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    BoneCBData boneData;
    UINT boneCount = static_cast<UINT>(boneMatrices->size());
    for (UINT i = 0; i < MAX_BONES; ++i)
    {
        if (i < boneCount)
            XMStoreFloat4x4(&boneData.BoneMatrices[i], XMMatrixTranspose(D3D12RendererToXM((*boneMatrices)[i])));
        else
            XMStoreFloat4x4(&boneData.BoneMatrices[i], XMMatrixIdentity());
    }

    UINT8* boneDest = fr.boneCBMapped + mObjectDrawIndex * fr.boneCBStride;
    memcpy(boneDest, &boneData, sizeof(BoneCBData));

    D3D12_GPU_VIRTUAL_ADDRESS boneCBAddress = fr.boneCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.boneCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(RootSlot::BoneCB), boneCBAddress);
}

void D3D12Renderer::DrawRenderItem(const RenderItem& item)
{
    if (mObjectDrawIndex >= kMaxObjectsPerFrame) return;

    ID3D12PipelineState* pso = item.IsSkinned() ? mSkinnedPSO.Get() : mPSO.Get();
    mCommandList->SetPipelineState(pso);

    BindItemTextures(item);
    UploadObjectCB(item);

    if (item.IsSkinned())
    {
        UploadBoneCB(item.BoneMatrices); //본 행렬 상수 버퍼 업로드
        if (item.SkinnedMeshPtr)
        {
            const_cast<SkinnedMesh*>(item.SkinnedMeshPtr)->TransitionToRenderState(mCommandList.Get());
            mCommandList->IASetVertexBuffers(0, 1, &item.SkinnedMeshPtr->GetVertexBufferView());
            mCommandList->IASetIndexBuffer(&item.SkinnedMeshPtr->GetIndexBufferView());
            UINT count = (item.IndexCount > 0) ? item.IndexCount : item.SkinnedMeshPtr->GetIndexCount();
            mCommandList->DrawIndexedInstanced(count, 1, item.IndexStart, item.BaseVertexLocation, 0);
        }
    }
    else if (item.MeshPtr)
    {
        DrawGeometry(*item.MeshPtr, item.IndexCount, item.IndexStart, item.BaseVertexLocation);
    }

    mObjectDrawIndex++;
}

//cascadeIndex를 매개변수로 받아 지정된 Layer DSV로 바인딩
void D3D12Renderer::BeginShadowPass(const DirectionalLight& light, const Camera& camera, UINT cascadeIndex)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    if (cascadeIndex == 0)
    {
        if (!mIsCommandListRecording)
        {
            SyncFrameViaRingBuffer(fr);
            ResetCommandSession(fr);
            mIsCommandListRecording = true;
        }

        //CSMCalculator 분리 호출 및 3단계 ViewProj 저장
        CascadeData csmData = CSMCalculator::Calculate(light, camera, 0.1f, 100.0f, mShadowMap.GetSize());
        for (UINT i = 0; i < kCascadeCount; ++i)
        {
            mCascadeLightViewProj[i] = csmData.ViewProjMatrices[i];
        }
        mCascadeSplits = csmData.SplitDistances;
    }

    //현재 패스의 LightViewProj 지정
    mLightViewProj = mCascadeLightViewProj[cascadeIndex];

    //D32_FLOAT 전용 Texture2DArray 지정 Layer DSV 바인딩
    mShadowMap.BeginPass(mCommandList.Get(), cascadeIndex);

    //D32_FLOAT 전용 Shadow RootSignature & PSO 바인딩
    mCommandList->SetGraphicsRootSignature(mShadowRootSignature.Get());
    mCommandList->SetPipelineState(mShadowPSO.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D12Renderer::DrawShadowMesh(const Mesh& mesh, const Matrix4x4& worldMatrix, UINT indexCount, UINT indexStart, INT baseVertexLocation)
{
    if (mShadowDrawIndex >= kMaxObjectsPerFrame) return;

    const_cast<Mesh&>(mesh).TransitionToRenderState(mCommandList.Get());

    //Shadow Pass용 D32_FLOAT PSO
    mCommandList->SetPipelineState(mShadowPSO.Get());

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ShadowCBData cbData;
    XMStoreFloat4x4(&cbData.WorldLightViewProj, XMMatrixTranspose(D3D12RendererToXM(worldMatrix * mLightViewProj)));

    UINT8* dest = fr.shadowCBMapped + mShadowDrawIndex * fr.shadowCBStride;
    memcpy(dest, &cbData, sizeof(cbData));

    D3D12_GPU_VIRTUAL_ADDRESS addr = fr.shadowCB->GetGPUVirtualAddress() + mShadowDrawIndex * fr.shadowCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(ShadowRootSlot::ShadowCB), addr);

    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    UINT count = (indexCount > 0) ? indexCount : mesh.GetIndexCount();
    mCommandList->DrawIndexedInstanced(count, 1, indexStart, baseVertexLocation, 0);

    mShadowDrawIndex++;
}

void D3D12Renderer::DrawShadowSkinnedMesh(const SkinnedMesh& mesh, const Matrix4x4& worldMatrix, const std::vector<Matrix4x4>& boneMatrices, UINT indexCount, UINT indexStart, INT baseVertexLocation)
{
    if (mShadowDrawIndex >= kMaxObjectsPerFrame) return;

    const_cast<SkinnedMesh&>(mesh).TransitionToRenderState(mCommandList.Get());

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    BoneCBData boneData = {};
    UINT boneCount = static_cast<UINT>(boneMatrices.size());
    for (UINT i = 0; i < MAX_BONES; ++i)
    {
        if (i < boneCount)
            XMStoreFloat4x4(&boneData.BoneMatrices[i], XMMatrixTranspose(D3D12RendererToXM(boneMatrices[i])));
        else
            XMStoreFloat4x4(&boneData.BoneMatrices[i], XMMatrixIdentity());
    }

    UINT8* boneDest = fr.boneCBMapped + mShadowDrawIndex * fr.boneCBStride;
    memcpy(boneDest, &boneData, sizeof(BoneCBData));

    D3D12_GPU_VIRTUAL_ADDRESS boneCBAddress = fr.boneCB->GetGPUVirtualAddress() + mShadowDrawIndex * fr.boneCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(ShadowRootSlot::BoneCB), boneCBAddress);

    ShadowCBData cbData = {};
    XMStoreFloat4x4(&cbData.WorldLightViewProj, XMMatrixTranspose(D3D12RendererToXM(worldMatrix * mLightViewProj)));

    UINT8* shadowDest = fr.shadowCBMapped + mShadowDrawIndex * fr.shadowCBStride;
    memcpy(shadowDest, &cbData, sizeof(cbData));

    D3D12_GPU_VIRTUAL_ADDRESS shadowCBAddress = fr.shadowCB->GetGPUVirtualAddress() + mShadowDrawIndex * fr.shadowCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(ShadowRootSlot::ShadowCB), shadowCBAddress);

    mCommandList->SetPipelineState(mShadowSkinnedPSO.Get());

    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    UINT count = (indexCount > 0) ? indexCount : mesh.GetIndexCount();
    mCommandList->DrawIndexedInstanced(count, 1, indexStart, baseVertexLocation, 0);

    mShadowDrawIndex++;
}

void D3D12Renderer::EndShadowPass()
{
    mShadowMap.EndPass(mCommandList.Get());
    mShadowDrawIndex = 0;
}

void D3D12Renderer::Render(const RenderQueue& renderQueue)
{
    for (const auto& item : renderQueue.GetOpaqueItems())
    {
        DrawRenderItem(item);
    }
}

void D3D12Renderer::RenderShadow(const RenderQueue& renderQueue)
{
    for (const auto& item : renderQueue.GetOpaqueItems())
    {
        if (item.IsSkinned())
        {
            if (item.SkinnedMeshPtr)
            {
                static const std::vector<Matrix4x4> kEmptyBones;
                const auto& bones = item.BoneMatrices ? *item.BoneMatrices : kEmptyBones;
                DrawShadowSkinnedMesh(*item.SkinnedMeshPtr, item.WorldMatrix, bones, item.IndexCount, item.IndexStart, item.BaseVertexLocation);
            }
        }
        else if (item.MeshPtr)
        {
            DrawShadowMesh(*item.MeshPtr, item.WorldMatrix, item.IndexCount, item.IndexStart, item.BaseVertexLocation);
        }
    }
}