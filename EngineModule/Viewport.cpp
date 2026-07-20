#include "pch.h"
#include "Viewport.h"
#include "imgui.h"

void Viewport::Draw(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, float& outWidth, float& outHeight, float& outPosX, float& outPosY)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    ImVec2 size = ImGui::GetContentRegionAvail();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    outWidth = size.x;
    outHeight = size.y;
    outPosX = pos.x;
    outPosY = pos.y;

    ImGui::Image((ImTextureID)textureHandle.ptr, size);

    ImGui::End();
    ImGui::PopStyleVar();
}