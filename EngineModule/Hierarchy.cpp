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

    ImGui::End();
}