#pragma once
#include <d3d12.h>
#include <windows.h>

class ImGuiLayer
{
public:
    void Initialize(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT frameCount, DXGI_FORMAT rtvFormat,
        ID3D12DescriptorHeap* srvHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuHandle);
    void Shutdown();

    void NewFrame();
    void Render(ID3D12GraphicsCommandList* commandList);

private:
    bool mInitialized = false;
};