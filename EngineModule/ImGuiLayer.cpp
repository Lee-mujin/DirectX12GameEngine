#include "pch.h"
#include "ImGuiLayer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

void ImGuiLayer::Initialize(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT frameCount, DXGI_FORMAT rtvFormat,
    ID3D12DescriptorHeap* srvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuHandle)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device;
    initInfo.CommandQueue = commandQueue;
    initInfo.NumFramesInFlight = static_cast<int>(frameCount);
    initInfo.RTVFormat = rtvFormat;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = srvHeap;
    initInfo.LegacySingleSrvCpuDescriptor = fontSrvCpuHandle;
    initInfo.LegacySingleSrvGpuDescriptor = fontSrvGpuHandle;

    ImGui_ImplDX12_Init(&initInfo);

    mInitialized = true;
}

void ImGuiLayer::Shutdown()
{
    if (!mInitialized)
    {
        return;
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::NewFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::Render(ID3D12GraphicsCommandList* commandList)
{
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}