#include "pch.h"
#include "Hierarchy.h"
#include "Scene.h"
#include "GameObject.h"
#include "EditorState.h"
#include "imgui.h"

void Hierarchy::Draw(Scene& scene, EditorState& editorState)
{
    ImGui::Begin("Hierarchy");

    auto selected = editorState.GetSelected();

    for (auto& obj : scene.GetGameObjects())
    {
        bool isSelected = (obj == selected);
        if (ImGui::Selectable(obj->GetName().c_str(), isSelected))
        {
            editorState.SetSelected(obj);
        }
    }

    // 변경점: 드롭 타겟 역시 성공(true) 시에만 EndDragDropTarget()을 호출합니다.
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET"))
        {
            std::string pathStr(static_cast<const char*>(payload->Data));
            if (mOnAssetDropped)
            {
                mOnAssetDropped(pathStr);
            }
        }
        ImGui::EndDragDropTarget(); // 반드시 이 조건문 scope 안에 고정
    }

    ImGui::End();
}