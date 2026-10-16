#include "pch.h"
#include "D3D12Renderer.h"
#include "MathUtil.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "ImGuiLayer.h"

using namespace DirectX;

//자체 Matrix4x4에서 DirectXMath XMMATRIX 포장 변환 헬퍼
//XMMATRIX를 쓰면 DirectX에 엔진 전체가 묶여버리고 쓰기도 까다롭기 때문에, 엔진 내부에서는 쓰기 편한Matrix4x4 자체 Matrix4x4 구조체를 사용
//DirectXMath 함수는 XMMATRIX 타입만 받아와 GPU 업로드,DirectX 연산시 XMMATRIX로 변환해 주는 다리 역할
XMMATRIX D3D12RendererToXM(const Matrix4x4& m)
{
    return XMMatrixSet(
        m._11, m._12, m._13, m._14,
        m._21, m._22, m._23, m._24,
        m._31, m._32, m._33, m._34,
        m._41, m._42, m._43, m._44);
}

//GPU 리소스 용도 전환 래퍼 함수
//GPU 자원이 현재 렌더링 타겟인지 텍스처 읽기용인지 직접 제어
//상태를 안 바꾸고 접근하면 GPU 충돌 발생해서 방지턱을 만듬
void D3D12Renderer::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
    mCommandList->ResourceBarrier(1, &barrier);
}

//링버퍼 방식의 CPU-GPU 동기화
//GPU가 아직 그리고 있는 프레임 리소스 버퍼를 덮어쓰는 메모리 오염 방지
//해당 프레임을 GPU가 다 쓸 때까지 CPU 스레드를 멈춰 대기
void D3D12Renderer::SyncFrameViaRingBuffer(FrameResource& fr)
{
    //fenceValue가 0이 아니면 이전에 GPU에 작업이 제출된 프레임임
    if (fr.fenceValue != 0)
    {
        //GPU가 완료한 펜스 값이 이 프레임 목표값보다 작다면 아직 GPU가 돌아가고있므로 CPU를 멈춰 대기
        if (mFence->GetCompletedValue() < fr.fenceValue)
        {
            ThrowIfFailed(mFence->SetEventOnCompletion(fr.fenceValue, mFenceEvent));
            WaitForSingleObject(mFenceEvent, INFINITE); //GPU 작업 완료 시까지 CPU 일시정지
        }
    }
    else
    {
        //첫 프레임 초기화 시점에는 안전하게 전체 GPU 명령 비우기
        WaitForGpu();
    }
}

//새 프레임용 커맨드 리스트/할당자 리셋
//셰이더 리소스를 커맨드 리스트에 재바인딩
void D3D12Renderer::ResetCommandSession(FrameResource& fr)
{
    ThrowIfFailed(fr.commandAllocator->Reset()); //커맨드 메모리 할당자 재사용
    ThrowIfFailed(mCommandList->Reset(fr.commandAllocator.Get(), nullptr)); //커맨드 리스트 기록

    //셰이더가 참고할 SRV 바인딩
    ID3D12DescriptorHeap* heaps[] = { mSrvAllocator.GetHeap() };
    mCommandList->SetDescriptorHeaps(1, heaps);
}

//3D 오프스크린 뷰포트 화방 세팅
//영역을 잡아서 화면 출력 범위 설정, 이전 프레임에 그려진 화면 잔상을 깨끗이 지움
//파이프라인 상태(PSO, RootSignature)를 바인딩해 삼각형을 그릴 준비
void D3D12Renderer::ConfigureViewportRenderTarget()
{
    RenderTarget& target = (mHDRRenderTarget.GetColorResource() != nullptr) ? mHDRRenderTarget : mViewportTarget;

    D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(target.GetWidth()), static_cast<float>(target.GetHeight()), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(target.GetWidth()), static_cast<LONG>(target.GetHeight()) };

    mCommandList->RSSetViewports(1, &vp);
    mCommandList->RSSetScissorRects(1, &scissor);

    //HDR 렌더 타깃 텍스처를 셰이더 읽기용에서 렌더타겟으로 용도 변경
    TransitionResource(target.GetColorResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = target.GetRtvHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = target.GetDsvHandle();

    //Output Merger 단계에 RTV(색상: R16G16B16A16_FLOAT) 및 DSV(깊이) 타겟 설정
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    mCommandList->ClearRenderTargetView(rtv, mClearColor, 0, nullptr); //배경색으로 지우기
    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr); //깊이 초기화

    //3D 셰이더 파이프라인 및 삼각형 출력 토폴로지 설정
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetPipelineState(mPSO.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//카메라 정보(View, Proj, Pos) GPU 업로드
//VS에서 3D정점을 카메라 시점 기준 2D 화면 좌표로 투영하기 위해 사용
//셰이더 메모리 규격에 맞춰 행렬을 Transpose시킨 후 GPU 상수버퍼로 복사
void D3D12Renderer::UpdateCameraConstantBuffer(FrameResource& fr, const Camera& camera)
{
    CameraCBData camData;
    //행렬을 셰이더 규격에 맞춰서 복사
    XMStoreFloat4x4(&camData.View, XMMatrixTranspose(D3D12RendererToXM(camera.GetView())));
    XMStoreFloat4x4(&camData.Proj, XMMatrixTranspose(D3D12RendererToXM(camera.GetProjection())));
    Vector3 camPos = camera.GetPosition();
    camData.CameraPosition = XMFLOAT3(camPos.X, camPos.Y, camPos.Z);
    camData.Pad0 = 0.0f; //16바이트 바운더리 패딩 정렬

    memcpy(fr.cameraCBMapped, &camData, sizeof(camData)); //GPU 매핑 메모리에 카피

    //Root Slot 1번에 카메라 상수 버퍼 GPU 주소 전달
    mCommandList->SetGraphicsRootConstantBufferView(1, fr.cameraCB->GetGPUVirtualAddress());
}

//조명 및 CSM 그림자 행렬 정보 GPU 업로드
//Directional, Point, Spot 조명의 색상/위치/감쇄율을 Pixel Shader로 전달해 음영을 계산
//Shadow Map그림자 계산용 단계별 Light ViewProj 행렬 데이터를 전달
void D3D12Renderer::UpdateLightConstantBuffer(FrameResource& fr, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight)
{
    LightCBData lightData = {};

    //Directional Light 세팅
    lightData.DirLightDirection = XMFLOAT3(dirLight.Direction.X, dirLight.Direction.Y, dirLight.Direction.Z);
    lightData.DirLightIntensity = dirLight.Intensity;
    lightData.DirLightColor = XMFLOAT3(dirLight.Color.X, dirLight.Color.Y, dirLight.Color.Z);

    //CSM 그림자 분할 단계별 행렬 배열 복사
    for (UINT i = 0; i < kNumCascades; ++i)
    {
        XMStoreFloat4x4(&lightData.WorldLightViewProj[i], XMMatrixTranspose(D3D12RendererToXM(mCascadeLightViewProj[i])));
    }
    lightData.CascadeSplits = XMFLOAT4(mCascadeSplits.X, mCascadeSplits.Y, mCascadeSplits.Z, 0.0f);

    //Point Light 세팅
    lightData.PointLightPosition = XMFLOAT3(pointLight.Position.X, pointLight.Position.Y, pointLight.Position.Z);
    lightData.PointLightIntensity = pointLight.Intensity;
    lightData.PointLightColor = XMFLOAT3(pointLight.Color.X, pointLight.Color.Y, pointLight.Color.Z);
    lightData.PointLightRange = pointLight.Range;
    lightData.PointLightEnabled = pointLight.Enabled ? 1 : 0;

    //Spot Light 세팅
    lightData.SpotLightPosition = XMFLOAT3(spotLight.Position.X, spotLight.Position.Y, spotLight.Position.Z);
    lightData.SpotLightIntensity = spotLight.Intensity;
    lightData.SpotLightDirection = XMFLOAT3(spotLight.Direction.X, spotLight.Direction.Y, spotLight.Direction.Z);
    lightData.SpotLightRange = spotLight.Range;
    lightData.SpotLightColor = XMFLOAT3(spotLight.Color.X, spotLight.Color.Y, spotLight.Color.Z);
    lightData.SpotLightInnerCos = cosf(Math::Deg2Rad(spotLight.InnerConeAngle)); // 셰이더 내적 연산용 cos값 변환
    lightData.SpotLightOuterCos = cosf(Math::Deg2Rad(spotLight.OuterConeAngle));
    lightData.SpotLightEnabled = spotLight.Enabled ? 1 : 0;

    Matrix4x4 spotVP = spotLight.GetLightViewProj();
    XMStoreFloat4x4(&lightData.SpotLightViewProj, XMMatrixTranspose(D3D12RendererToXM(spotVP)));

    memcpy(fr.lightCBMapped, &lightData, sizeof(lightData)); //GPU 매핑 메모리에 카피

    //Root Slot 2번에 조명 상수 버퍼 GPU 주소 전달
    mCommandList->SetGraphicsRootConstantBufferView(2, fr.lightCB->GetGPUVirtualAddress());
}

//메인 프레임 시작 수명주기 관리
//GPU 동기화 확인 -> 커맨드리스트 리셋 -> 뷰포트 초기화 -> 카메라/조명 CBuffer 업로드
void D3D12Renderer::BeginFrame(const Camera& camera, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight)
{
    FrameResource& fr = mFrameResources[mCurrentFrameResourceIndex];

    if (!mIsCommandListRecording)
    {
        SyncFrameViaRingBuffer(fr); //CPU-GPU 동기화 대기
        ResetCommandSession(fr); //커맨드 세션 초기화
        mIsCommandListRecording = true;
    }

    ConfigureViewportRenderTarget(); //화면 화방 세팅
    UpdateCameraConstantBuffer(fr, camera); //카메라 CB 업로드
    UpdateLightConstantBuffer(fr, dirLight, pointLight, spotLight); //조명 CB 업로드

    mObjectDrawIndex = 0; //드로우콜 카운터 초기화
}

//메인 프레임 종료 및 화면 제출
//ImGui 에디터 UI 그리기 명령 합치기
//그려진 백버퍼 텍스처 상태를 화면 출력용으로 전환
//기록 완료된 커맨드 리스트를 GPU 명령 큐에 일괄 제출
//모니터에 화면 출력 및 다음 링버퍼 프레임 자원 인덱스로 교체
void D3D12Renderer::EndFrame()
{
    //ImGui 에디터 UI 커맨드 기록
    if (mImGuiLayer) mImGuiLayer->Render(mCommandList.Get());

    //백버퍼 상태를 화면 출력용으로 전환
    TransitionResource(mBackBuffers[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    //커맨드 리스트 기록 종료
    ThrowIfFailed(mCommandList->Close());

    //실제 렌더링 실행
    ID3D12CommandList* commandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    //스왑체인 디스플레이 출력
    ThrowIfFailed(mSwapChain->Present(1, 0));

    //펜스 신호를 보내 이번 프레임 제출 완료 시점 기록
    mFenceValue++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

    mFrameResources[mCurrentFrameResourceIndex].fenceValue = mFenceValue;

    //다음 백버퍼 및 Ring Buffer 순환 0 -> 1 -> 2 -> 0
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % kFrameCount;

    mIsCommandListRecording = false;
}

//GPU 일시 정지 및 완전 동기화
//창 크기 리사이즈나 프로그램 종료 시, GPU가 열심히 일하는 중에 자원(Buffer/Texture)을 
//해제해 버리면 DX12 충돌이 나므로 GPU가 모든 일처리를 마칠 때까지 CPU를 강제 대기
void D3D12Renderer::WaitForGpu()
{
    mFenceValue++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

    //GPU가 방금 보낸 펜스 신호 지점까지 완수할 때까지 대기
    if (mFence->GetCompletedValue() < mFenceValue)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

//렌더러 파괴 시 자원 해
//GPU 작업을 WaitForGpu하고 Win32 동기화 이벤트 핸들을 파괴하여 메모리 누수를 방지하고 엔진을 종료
void D3D12Renderer::Cleanup()
{
    WaitForGpu(); //GPU가 남은 일을 다 끝낼 때까지 대기

    if (mFenceEvent)
    {
        CloseHandle(mFenceEvent); //Win32 이벤트 핸들 해제
        mFenceEvent = nullptr;
    }
}