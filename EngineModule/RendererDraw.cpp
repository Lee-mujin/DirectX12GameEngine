#include "pch.h"
#include "D3D12Renderer.h"
#include "MathUtil.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "SkinnedMesh.h"
#include "DirectionalLight.h"

using namespace DirectX;
extern XMMATRIX D3D12RendererToXM(const Matrix4x4& m);

void D3D12Renderer::DrawMeshInternal(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const BoneCBData* boneData, ID3D12PipelineState* pso)
{
    if (mObjectDrawIndex >= kMaxObjectsPerFrame) return;

    const_cast<Mesh&>(mesh).TransitionToRenderState(mCommandList.Get());
    mCommandList->SetPipelineState(pso);

    std::shared_ptr<Texture> baseTexture = material.GetTexture();
    if (!baseTexture) baseTexture = mDefaultWhiteTexture;

    if (baseTexture)
    {
        const_cast<Texture&>(*baseTexture).TransitionToRenderState(mCommandList.Get());
        mCommandList->SetGraphicsRootDescriptorTable(4, baseTexture->GetSrvHandle());
    }

    std::shared_ptr<Texture> normalTexture = material.GetNormalTexture();
    bool hasNormalMap = (normalTexture != nullptr);

    if (hasNormalMap)
    {
        const_cast<Texture&>(*normalTexture).TransitionToRenderState(mCommandList.Get());
        mCommandList->SetGraphicsRootDescriptorTable(5, normalTexture->GetSrvHandle());
    }
    else if (mDefaultWhiteTexture)
    {
        mCommandList->SetGraphicsRootDescriptorTable(5, mDefaultWhiteTexture->GetSrvHandle());
    }

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
    mCommandList->SetGraphicsRootConstantBufferView(0, objectCBAddress);

    if (boneData)
    {
        UINT8* boneDest = fr.boneCBMapped + mObjectDrawIndex * fr.boneCBStride;
        memcpy(boneDest, boneData, sizeof(BoneCBData));

        D3D12_GPU_VIRTUAL_ADDRESS boneCBAddress = fr.boneCB->GetGPUVirtualAddress() + mObjectDrawIndex * fr.boneCBStride;
        mCommandList->SetGraphicsRootConstantBufferView(3, boneCBAddress);
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

void D3D12Renderer::BeginShadowPass(const DirectionalLight& light)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];
    SyncFrameViaRingBuffer(fr);
    ResetCommandSession(fr);
    mIsCommandListRecording = true;

    Matrix4x4 lightView = light.GetLightView(Vector3::Zero);
    Matrix4x4 lightProj = light.GetLightProjection();
    mLightViewProj = lightView * lightProj;

    mShadowMap.Begin(mCommandList.Get());
    mCommandList->SetGraphicsRootSignature(mShadowRootSignature.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D12Renderer::DrawShadowMesh(const Mesh& mesh, const Matrix4x4& worldMatrix)
{
    if (mShadowDrawIndex >= kMaxObjectsPerFrame) return;

    const_cast<Mesh&>(mesh).TransitionToRenderState(mCommandList.Get());
    mCommandList->SetPipelineState(mShadowPSO.Get());

    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    ShadowCBData cbData;
    XMStoreFloat4x4(&cbData.WorldLightViewProj, XMMatrixTranspose(D3D12RendererToXM(worldMatrix * mLightViewProj)));

    UINT8* dest = fr.shadowCBMapped + mShadowDrawIndex * fr.shadowCBStride;
    memcpy(dest, &cbData, sizeof(cbData));

    D3D12_GPU_VIRTUAL_ADDRESS addr = fr.shadowCB->GetGPUVirtualAddress() + mShadowDrawIndex * fr.shadowCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(0, addr);

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
    mCommandList->SetGraphicsRootConstantBufferView(1, boneCBAddress);

    ShadowCBData cbData = {};
    XMStoreFloat4x4(&cbData.WorldLightViewProj, XMMatrixTranspose(D3D12RendererToXM(worldMatrix * mLightViewProj)));

    UINT8* shadowDest = fr.shadowCBMapped + mShadowDrawIndex * fr.shadowCBStride;
    memcpy(shadowDest, &cbData, sizeof(cbData));

    D3D12_GPU_VIRTUAL_ADDRESS shadowCBAddress = fr.shadowCB->GetGPUVirtualAddress() + mShadowDrawIndex * fr.shadowCBStride;
    mCommandList->SetGraphicsRootConstantBufferView(0, shadowCBAddress);

    mCommandList->SetPipelineState(mShadowSkinnedPSO.Get());

    mCommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
    mCommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());
    mCommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

    mShadowDrawIndex++;
}

void D3D12Renderer::EndShadowPass()
{
    mShadowMap.End(mCommandList.Get());
    mShadowDrawIndex = 0;
}