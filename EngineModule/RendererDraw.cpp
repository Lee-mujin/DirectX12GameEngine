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

//머티리얼 텍스처 및 섀도우 맵 루트 디스크립터 바인딩
void D3D12Renderer::BindMaterial(const Material& material, bool hasNormalMap)
{
    //Base Color Texture 바인딩
    std::shared_ptr<Texture> baseTexture = material.GetTexture();
    if (!baseTexture) baseTexture = mDefaultWhiteTexture;

    if (baseTexture)
    {
        const_cast<Texture&>(*baseTexture).TransitionToRenderState(mCommandList.Get());
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::BaseTexture), baseTexture->GetSrvHandle());
    }

    //Normal Map Texture 바인딩
    std::shared_ptr<Texture> normalTexture = material.GetNormalTexture();
    if (hasNormalMap && normalTexture)
    {
        const_cast<Texture&>(*normalTexture).TransitionToRenderState(mCommandList.Get());
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::NormalTexture), normalTexture->GetSrvHandle());
    }
    else if (mDefaultWhiteTexture)
    {
        mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::NormalTexture), mDefaultWhiteTexture->GetSrvHandle());
    }

    //6번 Root Slot에 Shadow Map SRV 바인딩
    mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::ShadowTexture), mShadowMap.GetSRV());
}

//오브젝트 상수 버퍼(ObjectCB) 메모리 복사 및 Root CBV 바인딩
void D3D12Renderer::UploadObjectCB(const Material& material, const Matrix4x4& worldMatrix, bool hasNormalMap)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ObjectCBData objData;
    XMStoreFloat4x4(&objData.World, XMMatrixTranspose(D3D12RendererToXM(worldMatrix)));

    Vector3 matColor = material.GetBaseColor();
    objData.MaterialColor = XMFLOAT3(matColor.X, matColor.Y, matColor.Z);
    objData.Shininess = material.GetShininess();
    objData.HasNormalMap = hasNormalMap ? 1 : 0;

    UINT8* objDest = fr.objectCBMapped + mObjectDrawIndex * fr.objectCBStride;
    memcpy(objDest, &objData, sizeof(objData));

    D3D12_GPU_VIRTUAL_ADDRESS objectCBAddress = fr.objectCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.objectCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(RootSlot::ObjectCB), objectCBAddress);
}

//스킨드 애니메이션용 보간 행렬(BoneCB) 데이터 업로드
void D3D12Renderer::UploadBoneCB(const BoneCBData* boneData)
{
    if (!boneData) return;

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    UINT8* boneDest = fr.boneCBMapped + mObjectDrawIndex * fr.boneCBStride;
    memcpy(boneDest, boneData, sizeof(BoneCBData));

    D3D12_GPU_VIRTUAL_ADDRESS boneCBAddress = fr.boneCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.boneCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(RootSlot::BoneCB), boneCBAddress);
}

//메시 상태 전이 및 인덱스 기반 DrawCall 수행
void D3D12Renderer::DrawGeometry(const Mesh& mesh)
{
    const_cast<Mesh&>(mesh).TransitionToRenderState(mCommandList.Get());
    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    mCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);
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
    objData.HasNormalMap = item.HasNormalMap ? 1 : 0;

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
        UploadBoneCB(item.BoneMatrices);
        if (item.SkinnedMeshPtr)
        {
            const_cast<SkinnedMesh*>(item.SkinnedMeshPtr)->TransitionToRenderState(mCommandList.Get());
            mCommandList->IASetVertexBuffers(0, 1, &item.SkinnedMeshPtr->GetVertexBufferView());
            mCommandList->IASetIndexBuffer(&item.SkinnedMeshPtr->GetIndexBufferView());
            mCommandList->DrawIndexedInstanced(item.SkinnedMeshPtr->GetIndexCount(), 1, 0, 0, 0);
        }
    }
    else if (item.MeshPtr)
    {
        DrawGeometry(*item.MeshPtr);
    }

    mObjectDrawIndex++;
}

void D3D12Renderer::DrawMeshInternal(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const BoneCBData* boneData, ID3D12PipelineState* pso)
{
    if (mObjectDrawIndex >= kMaxObjectsPerFrame) return;

    mCommandList->SetPipelineState(pso);

    bool hasNormalMap = (material.GetNormalTexture() != nullptr);

    BindMaterial(material, hasNormalMap);
    UploadObjectCB(material, worldMatrix, hasNormalMap);
    UploadBoneCB(boneData);
    DrawGeometry(mesh);

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

void D3D12Renderer::DrawShadowMesh(const Mesh& mesh, const Matrix4x4& worldMatrix)
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
    mCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

    mShadowDrawIndex++;
}

void D3D12Renderer::DrawShadowSkinnedMesh(const SkinnedMesh& mesh, const Matrix4x4& worldMatrix, const std::vector<Matrix4x4>& boneMatrices)
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
    mCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

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
                DrawShadowSkinnedMesh(*item.SkinnedMeshPtr, item.WorldMatrix, bones);
            }
        }
        else if (item.MeshPtr)
        {
            DrawShadowMesh(*item.MeshPtr, item.WorldMatrix);
        }
    }
}