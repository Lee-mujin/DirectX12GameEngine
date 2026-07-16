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

    // 1. 오브젝트 리스트 출력 루프
    for (auto& obj : scene.GetGameObjects())
    {
        ImGui::PushID(obj.get());

        bool isSelected = (obj == selected);
        if (ImGui::Selectable(obj->GetName().c_str(), isSelected))
        {
            editorState.SetSelected(obj);
        }

        // 개별 오브젝트 위에 직접 드롭했을 때 (예: 계층 변경이나 특정 타겟 지정 시 사용 가능)
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
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
    }

    //빈 공간 확보 및 창 전체 드롭 타겟 처리 추가
    //Dummy추가 아이템 리스트 아래쪽 빈 공간 전체를 마우스 호버 가능한 더미 영역으로 채움
    //마우스가 아이템 아래 빈 공간에 머무를 때 안전하게 드롭
    ImGui::Dummy(ImGui::GetContentRegionAvail());

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
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}