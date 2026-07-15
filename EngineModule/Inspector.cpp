#include "pch.h"
#include "Inspector.h"
#include "EditorState.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "imgui.h"
#include "Animation.h"

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

    if (AnimatorComponent* animator = selected->GetComponent<AnimatorComponent>())
    {
        if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& animList = animator->GetAnimationList();

            if (!animList.empty())
            {
                std::string previewName = animator->GetCurrentClipName();
                if (previewName.empty()) previewName = "(None)";

                if (ImGui::BeginCombo("Clip", previewName.c_str()))
                {
                    for (auto& clip : animList)
                    {
                        bool isSelected = (clip->Name == animator->GetCurrentClipName());
                        if (ImGui::Selectable(clip->Name.c_str(), isSelected))
                        {
                            animator->CrossFade(clip->Name, 0.3f); // 0.3초에 걸쳐 부드럽게 전환
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else
            {
                ImGui::Text("(No animation clips)");
            }

            bool isPlaying = animator->IsPlaying();
            if (isPlaying)
            {
                if (ImGui::Button("Pause")) animator->Pause();
            }
            else
            {
                if (ImGui::Button("Play")) animator->Resume();
            }

            ImGui::SameLine();
            if (ImGui::Button("Stop")) animator->Stop();

            float speed = animator->GetSpeed();
            if (ImGui::DragFloat("Speed", &speed, 0.05f, 0.0f, 5.0f))
            {
                animator->SetSpeed(speed);
            }

            bool loop = animator->IsLooping();
            if (ImGui::Checkbox("Loop", &loop))
            {
                animator->SetLoop(loop);
            }

            float duration = animator->GetDuration();
            float currentTime = animator->GetCurrentTime();

            if (ImGui::SliderFloat("Time", &currentTime, 0.0f, duration > 0.0f ? duration : 0.001f))
            {
                animator->SetCurrentTime(currentTime);
            }

            ImGui::Text("Duration: %.2f s", duration);
        }
    }

    ImGui::End();
}