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

using Microsoft::WRL::ComPtr;

class Mesh;
class Material;
class Camera;
class Texture;
class ImGuiLayer;
class SkinnedMesh;
struct Matrix4x4;
struct DirectionalLight;
struct PointLight;
struct SpotLight;

class D3D12Renderer
{
public:
    bool Initialize(HWND hwnd, UINT width, UINT height);
    void Cleanup();

    void BeginFrame(const Camera& camera, const DirectionalLight& dirLight, const PointLight& pointLight, const SpotLight& spotLight);
    void DrawMesh(const Mesh& mesh, const Material& material, const Matrix4x4& worldMatrix);
    void EndFrame();

    bool AllocateSrvSlot(D3D12_CPU_DESCRIPTOR_HANDLE& outCpu, D3D12_GPU_DESCRIPTOR_HANDLE& outGpu);
    void SetImGuiLayer(ImGuiLayer* layer) { mImGuiLayer = layer; }
    void DrawSkinnedMesh(const SkinnedMesh& mesh, const Material& material, const Matrix4x4& worldMatrix, const std::vector<Matrix4x4>& boneMatrices);

public:
    ID3D12Device* GetDevice() const { return mDevice.Get(); }
    std::shared_ptr<Texture> LoadTextureFromFile(const std::wstring& path);
    std::shared_ptr<Texture> CreateSolidColorTexture(BYTE r, BYTE g, BYTE b, BYTE a);
    ID3D12CommandQueue* GetCommandQueue() const { return mCommandQueue.Get(); }
    ID3D12DescriptorHeap* GetSrvHeap() const { return mSrvHeap.Get(); }
    static constexpr UINT GetFrameCount() { return kFrameCount; }


private:
    void CreateFactory();
    void CreateDevice();
    void CreateCommandQueue();
    void CreateCommandList();
    void CreateFrameResources();
    void CreateFence();
    void CreateSwapChain();
    void CreateDescriptorHeaps();
    void CreateSrvHeap();
    void CreateRenderTargetViews();
    void CreateDepthStencil();
    void SetupViewport();
    void CreateShader();
    void CreateRootSignature();
    void CreatePSO();
    void CreateSkinnedPSO();

    void WaitForGpu(); // Cleanup에서만 사용

private:
    static constexpr UINT kFrameCount = 2;
    static constexpr UINT kMaxObjectsPerFrame = 64;
    static constexpr UINT kMaxSrvCount = 32;

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

    ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    ComPtr<ID3D12DescriptorHeap> mDSVHeap;
    ComPtr<ID3D12DescriptorHeap> mSrvHeap;
    UINT mRTVDescriptorSize = 0;
    UINT mDSVDescriptorSize = 0;
    UINT mSrvDescriptorSize = 0;
    UINT mSrvUsedCount = 0;

    ComPtr<ID3D12Resource> mBackBuffers[kFrameCount];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPSO;
    ComPtr<ID3DBlob> mVertexShader;
    ComPtr<ID3DBlob> mPixelShader;

    ComPtr<ID3DBlob> mSkinnedVertexShader;
    ComPtr<ID3D12PipelineState> mSkinnedPSO;

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT mScissorRect{};

    UINT mFrameIndex = 0;

    ImGuiLayer* mImGuiLayer = nullptr;

    float mClearColor[4] = { 0.0f, 0.2f, 0.6f, 1.0f };
};