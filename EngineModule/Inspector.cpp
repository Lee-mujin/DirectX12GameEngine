#include "pch.h"
#include "Inspector.h"
#include "EditorState.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "imgui.h"

void Inspector::Draw(EditorState& editorState)
{
    ImGui::Begin("Inspector");

    auto selected = editorState.GetSelected();
    if (!selected)
    {
        ImGui::Text("(No object selected)");
        ImGui::End();
        return;
    }

    ImGui::Text("%s", selected->GetName().c_str());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Transform& transform = selected->GetTransform();

        Vector3 position = transform.GetPosition();
        if (ImGui::DragFloat3("Position", &position.X, 0.1f))
        {
            transform.SetPosition(position);
        }

        Vector3 rotation = transform.GetEulerAngles();
        if (ImGui::DragFloat3("Rotation", &rotation.X, 1.0f))
        {
            transform.SetRotation(rotation);
        }

        Vector3 scale = transform.GetScale();
        if (ImGui::DragFloat3("Scale", &scale.X, 0.05f))
        {
            transform.SetScale(scale);
        }
    }

    if (MeshRenderer* meshRenderer = selected->GetComponent<MeshRenderer>())
    {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto material = meshRenderer->GetMaterial();
            if (material)
            {
                Vector3 color = material->GetColor();
                if (ImGui::ColorEdit3("Albedo Color", &color.X))
                {
                    material->SetColor(color);
                }

                float shininess = material->GetShininess();
                if (ImGui::DragFloat("Shininess", &shininess, 1.0f, 1.0f, 256.0f))
                {
                    material->SetShininess(shininess);
                }

                auto texture = material->GetTexture();

                if (texture)
                {
                    ImGui::Text("Texture:");
                    ImGui::Image((ImTextureID)texture->GetSrvHandle().ptr, ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("Texture: (None)");
                }

                if (ImGui::Button("Change Texture..."))
                {
                    wchar_t filePath[MAX_PATH] = {};
                    OPENFILENAMEW ofn = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrFilter = L"Image Files\0*.png;*.jpg;*.bmp\0All Files\0*.*\0";
                    ofn.lpstrFile = filePath;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    if (GetOpenFileNameW(&ofn))
                    {
                        // material->SetTexture(resourceManager.LoadTexture(filePath));
                        // ResourceManager가 없어서 아직 비활성
                    }
                }
            }
            else
            {
                ImGui::Text("(No Material)");
            }
        }
    }

    ImGui::End();
}