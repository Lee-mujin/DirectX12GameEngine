#pragma once
#include <string>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <array>
#include <memory>
#include <vector>
#include "FrameResource.h"
#include "RenderTarget.h"
#include "DescriptorAllocator.h"
#include "ShadowMap.h"
#include "MathUtil.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "RenderItem.h"
#include "RenderQueue.h"

using Microsoft::WRL::ComPtr;

//루트 슬롯 번호 enum으로 상수화
enum class RootSlot : UINT
{
    ObjectCB = 0,
    CameraCB = 1,
    LightCB = 2,
    BoneCB = 3,
    BaseTexture = 4,
    NormalTexture = 5,
    ShadowTexture = 6
};

enum class ShadowRootSlot : UINT
{
    ShadowCB = 0,
    BoneCB = 1
};

class Mesh;
class Material;
class Camera;
class Texture;
class ImGuiLayer;
class SkinnedMesh;
struct DirectionalLight;
struct PointLight;
struct SpotLight;

class D3D12Renderer
{
public:
    bool Initialize(HWND hwnd, UINT width, UINT height);
    void Cleanup();
    DescriptorHandle AllocateSrvSlot()
    {
        return mSrvAllocator.Allocate();
    }

    const DescriptorAllocator& GetSrvAllocator() const { return mSrvAllocator; }
    DescriptorAllocator* GetSrvAllocatorPtr() { return &mSrvAllocator; }
    ID3D12DescriptorHeap* GetSrvHeap() const { return mSrvAllocator.GetHeap(); }

    void BeginFrame(const Camera& camera, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight);
    void DrawMesh(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix);
    void DrawSkinnedMesh(const SkinnedMesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const std::vector<Matrix4x4>& boneMatrices);

    // RenderItem 기반 드로우 함수 추가
    void DrawRenderItem(const RenderItem& item);

    void EndFrame();

    void SetImGuiLayer(ImGuiLayer* layer) { mImGuiLayer = layer; }
    void WaitForGpu();

    ID3D12Device* GetDevice() const { return mDevice.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return mCommandQueue.Get(); }
    static constexpr UINT GetFrameCount() { return kFrameCount; }

    void InitializeViewport(UINT width, UINT height);
    void ResizeViewport(UINT width, UINT height);
    D3D12_GPU_DESCRIPTOR_HANDLE GetViewportSrvHandle() const { return mViewportTarget.GetSrvHandle(); }

    void TransitionToSwapChain();
    void SetDefaultWhiteTexture(std::shared_ptr<Texture> texture) { mDefaultWhiteTexture = texture; }

    bool InitializeShadowMap(); //Initialize() 안에서 호출

    //CSM Cascade Index 지원을 위해 cascadeIndex = 0 매개변수 추가
    void BeginShadowPass(const DirectionalLight& light, const Camera& camera, UINT cascadeIndex = 0);
    void DrawShadowMesh(const Mesh& mesh, const Matrix4x4& worldMatrix);
    void DrawShadowSkinnedMesh(const SkinnedMesh& mesh, const Matrix4x4& worldMatrix, const std::vector<Matrix4x4>& boneMatrices);
    void EndShadowPass();

    void Render(const RenderQueue& renderQueue);
    void RenderShadow(const RenderQueue& renderQueue);

    Matrix4x4 GetLightViewProj() const { return mLightViewProj; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetShadowMapSRV() const { return mShadowMap.GetSRV(); }

private:
    // Initialize 도메인 모듈화 함수들
    void InitializeDevice();
    void InitializeFrameResources();
    void InitializeDescriptors();
    void InitializeRenderTargets();
    void InitializePipeline();

    void CompileShaderInternal(const std::wstring& filename, const char* entryPoint, const char* target, ID3DBlob** shaderBlob);

    // DrawMeshInternal 서브루틴 분리 함수 선언
    void BindMaterial(const Material& material, bool hasNormalMap);
    void UploadObjectCB(const Material& material, const Matrix4x4& worldMatrix, bool hasNormalMap);
    void UploadBoneCB(const BoneCBData* boneData);
    void DrawGeometry(const Mesh& mesh);

    // RenderItem 전용 서브루틴 선언
    void BindItemTextures(const RenderItem& item);
    void UploadObjectCB(const RenderItem& item);
    void UploadBoneCB(const std::vector<Matrix4x4>* boneMatrices);

    void DrawMeshInternal(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const BoneCBData* boneData, ID3D12PipelineState* pso);

    void SyncFrameViaRingBuffer(FrameResource& fr);
    void ResetCommandSession(FrameResource& fr);
    void ConfigureViewportRenderTarget();
    void UpdateCameraConstantBuffer(FrameResource& fr, const Camera& camera);
    void UpdateLightConstantBuffer(FrameResource& fr, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight);

    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    void CreatePipelineState(ID3DBlob* vsBlob, ID3DBlob* psBlob, ID3D12RootSignature* rootSig, DXGI_FORMAT dsvFormat, bool isShadowPSO, ID3D12PipelineState** outPSO);

    void CreateFactory();
    void CreateDevice();
    void CreateCommandQueue();
    void CreateCommandList();
    void CreateFrameResources();
    void CreateFence();
    void CreateSwapChain();
    void CreateRenderTargetViews();
    void CreateDepthStencil();
    void SetupViewport();
    void CreateShader();
    void CreateRootSignature();
    void CreatePSOs();

private:
    static constexpr UINT kFrameCount = 2;
    static constexpr UINT kMaxObjectsPerFrame = 64;
    static constexpr UINT kMaxSrvCount = 32;
    static constexpr UINT kCascadeCount = 3; //Cascade 개수 상수 정의

    HWND mHwnd = nullptr;
    UINT mWidth = 0;
    UINT mHeight = 0;

    ComPtr<IDXGIFactory6> mFactory;
    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mFenceValue = 0;
    HANDLE mFenceEvent = nullptr;

    std::array<FrameResource, kFrameCount> mFrameResources;
    UINT mCurrentFrameResourceIndex = 0;
    UINT mObjectDrawIndex = 0;

    ComPtr<IDXGISwapChain4> mSwapChain;

    DescriptorAllocator mSrvAllocator;
    DescriptorAllocator mRtvAllocator;
    DescriptorAllocator mDsvAllocator;

    ComPtr<ID3D12Resource> mBackBuffers[kFrameCount];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPSO;
    ComPtr<ID3D12PipelineState> mSkinnedPSO;

    ComPtr<ID3DBlob> mVertexShader;
    ComPtr<ID3DBlob> mPixelShader;
    ComPtr<ID3DBlob> mSkinnedVertexShader;

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT mScissorRect{};

    UINT mFrameIndex = 0;

    ImGuiLayer* mImGuiLayer = nullptr;

    RenderTarget mViewportTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE mViewportRtvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE mViewportDsvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE mViewportSrvCpuHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE mViewportSrvGpuHandle = { 0 };
    bool mIsViewportSrvAllocated = false;

    std::shared_ptr<Texture> mDefaultWhiteTexture;

    ShadowMap mShadowMap;
    ComPtr<ID3D12RootSignature> mShadowRootSignature;
    ComPtr<ID3D12PipelineState> mShadowPSO;
    ComPtr<ID3D12PipelineState> mShadowSkinnedPSO;
    ComPtr<ID3DBlob> mShadowVertexShader;
    ComPtr<ID3DBlob> mShadowSkinnedVertexShader;
    UINT mShadowDrawIndex = 0;
    Matrix4x4 mLightViewProj; //Main Pass 셰이더에 넘길 때 씀

    bool mIsCommandListRecording = false;

    Matrix4x4 mCascadeLightViewProj[3];
    Vector3 mCascadeSplits;

    float mClearColor[4] = { 0.0f, 0.2f, 0.6f, 1.0f };
};