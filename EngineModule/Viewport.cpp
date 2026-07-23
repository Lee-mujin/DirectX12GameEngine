#include "pch.h"
#include "Viewport.h"
#include "EditorState.h"
#include "imgui.h"

void Viewport::Draw(EditorState& editorState, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, float& outWidth, float& outHeight, float& outPosX, float& outPosY)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    //현재 Viewport 패널이 포커스(활성화) 상태인지 EditorState에 기록
    editorState.SetViewportFocused(ImGui::IsWindowFocused());

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