#include "pch.h"
#include "D3D12Renderer.h"
#include "MathUtil.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "ImGuiLayer.h"

using namespace DirectX;

//프레임 세션 헬퍼 영역으로 단일화
XMMATRIX D3D12RendererToXM(const Matrix4x4& m)
{
    return XMMatrixSet(
        m._11, m._12, m._13, m._14,
        m._21, m._22, m._23, m._24,
        m._31, m._32, m._33, m._34,
        m._41, m._42, m._43, m._44);
}

void D3D12Renderer::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
    mCommandList->ResourceBarrier(1, &barrier);
}

void D3D12Renderer::SyncFrameViaRingBuffer(FrameResource& fr)
{
    //fenceValue가 0이 아니고, GPU 처리 값(GetCompletedValue)보다 크면 대기
    if (fr.fenceValue != 0)
    {
        if (mFence->GetCompletedValue() < fr.fenceValue)
        {
            ThrowIfFailed(mFence->SetEventOnCompletion(fr.fenceValue, mFenceEvent));
            WaitForSingleObject(mFenceEvent, INFINITE);
        }
    }
    else
    {
        //첫 프레임 초기화 시점에 실행 중인 GPU 작업이 있다면 Flush
        WaitForGpu();
    }
}

void D3D12Renderer::ResetCommandSession(FrameResource& fr)
{
    ThrowIfFailed(fr.commandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(fr.commandAllocator.Get(), nullptr));

    ID3D12DescriptorHeap* heaps[] = { mSrvAllocator.GetHeap() };
    mCommandList->SetDescriptorHeaps(1, heaps);
}

void D3D12Renderer::ConfigureViewportRenderTarget()
{
    D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(mViewportTarget.GetWidth()), static_cast<float>(mViewportTarget.GetHeight()), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(mViewportTarget.GetWidth()), static_cast<LONG>(mViewportTarget.GetHeight()) };

    mCommandList->RSSetViewports(1, &vp);
    mCommandList->RSSetScissorRects(1, &scissor);

    TransitionResource(mViewportTarget.GetColorResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mViewportTarget.GetRtvHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = mViewportTarget.GetDsvHandle();

    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    mCommandList->ClearRenderTargetView(rtv, mClearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetPipelineState(mPSO.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D12Renderer::UpdateCameraConstantBuffer(FrameResource& fr, const Camera& camera)
{
    CameraCBData camData;
    XMStoreFloat4x4(&camData.View, XMMatrixTranspose(D3D12RendererToXM(camera.GetView())));
    XMStoreFloat4x4(&camData.Proj, XMMatrixTranspose(D3D12RendererToXM(camera.GetProjection())));
    Vector3 camPos = camera.GetPosition();
    camData.CameraPosition = XMFLOAT3(camPos.X, camPos.Y, camPos.Z);
    camData.Pad0 = 0.0f;
    memcpy(fr.cameraCBMapped, &camData, sizeof(camData));

    mCommandList->SetGraphicsRootConstantBufferView(1, fr.cameraCB->GetGPUVirtualAddress());
}

void D3D12Renderer::UpdateLightConstantBuffer(FrameResource& fr, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight)
{
    LightCBData lightData = {};
    lightData.DirLightDirection = XMFLOAT3(dirLight.Direction.X, dirLight.Direction.Y, dirLight.Direction.Z);
    lightData.DirLightIntensity = dirLight.Intensity;
    lightData.DirLightColor = XMFLOAT3(dirLight.Color.X, dirLight.Color.Y, dirLight.Color.Z);
    lightData.PointLightPosition = XMFLOAT3(pointLight.Position.X, pointLight.Position.Y, pointLight.Position.Z);
    lightData.PointLightIntensity = pointLight.Intensity;
    lightData.PointLightColor = XMFLOAT3(pointLight.Color.X, pointLight.Color.Y, pointLight.Color.Z);
    lightData.PointLightRange = pointLight.Range;
    lightData.PointLightEnabled = pointLight.Enabled ? 1 : 0;
    lightData.SpotLightPosition = XMFLOAT3(spotLight.Position.X, spotLight.Position.Y, spotLight.Position.Z);
    lightData.SpotLightIntensity = spotLight.Intensity;
    lightData.SpotLightDirection = XMFLOAT3(spotLight.Direction.X, spotLight.Direction.Y, spotLight.Direction.Z);
    lightData.SpotLightRange = spotLight.Range;
    lightData.SpotLightColor = XMFLOAT3(spotLight.Color.X, spotLight.Color.Y, spotLight.Color.Z);
    lightData.SpotLightInnerCos = cosf(Math::Deg2Rad(spotLight.InnerConeAngle));
    lightData.SpotLightOuterCos = cosf(Math::Deg2Rad(spotLight.OuterConeAngle));
    lightData.SpotLightEnabled = spotLight.Enabled ? 1 : 0;
    memcpy(fr.lightCBMapped, &lightData, sizeof(lightData));

    mCommandList->SetGraphicsRootConstantBufferView(2, fr.lightCB->GetGPUVirtualAddress());
}

void D3D12Renderer::BeginFrame(const Camera& camera, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    SyncFrameViaRingBuffer(fr);
    ResetCommandSession(fr);
    ConfigureViewportRenderTarget();
    UpdateCameraConstantBuffer(fr, camera);
    UpdateLightConstantBuffer(fr, dirLight, pointLight, spotLight);

    mObjectDrawIndex = 0;
}

void D3D12Renderer::EndFrame()
{
    if (mImGuiLayer) mImGuiLayer->Render(mCommandList.Get());

    TransitionResource(mBackBuffers[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* commandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ThrowIfFailed(mSwapChain->Present(1, 0));

    mFenceValue++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

    mFrameResources[mCurrentFrameResourceIndex].fenceValue = mFenceValue;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % kFrameCount;
}

void D3D12Renderer::WaitForGpu()
{
    mFenceValue++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

    if (mFence->GetCompletedValue() < mFenceValue)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

void D3D12Renderer::Cleanup()
{
    WaitForGpu();

    if (mFenceEvent)
    {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
}