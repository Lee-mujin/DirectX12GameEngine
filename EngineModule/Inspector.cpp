#include "pch.h"
#include "Inspector.h"
#include "EditorState.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "imgui.h"
#include "Animation.h"
#include "ResourceManager.h"
#include "AssetHandle.h"
#include "Model.h"
#include "AnimatorComponent.h"
#include "ModelUtility.h"
#include <optional>

namespace
{
    // 범용 에셋 드롭 타겟 처리 헬퍼 (Content Browser 연동)
    std::optional<AssetHandle> HandleAssetDropTarget(ResourceManager& resourceManager)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET"))
            {
                std::string droppedPath(static_cast<const char*>(payload->Data));
                AssetHandle handle = resourceManager.GetOrCreateHandle(droppedPath);

                ImGui::EndDragDropTarget();
                return handle;
            }
            ImGui::EndDragDropTarget();
        }
        return std::nullopt;
    }
}

void Inspector::Draw(EditorState& editorState, ResourceManager& resourceManager)
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

    ImGui::PushID(selected.get());

    // 1. Transform 영역
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

    // 2. Material 영역
    if (MeshRenderer* meshRenderer = selected->GetComponent<MeshRenderer>())
    {
        ImGui::PushID("MaterialComponent");

        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto material = meshRenderer->GetMaterial();
            if (material)
            {
                Vector3 baseColor = material->GetBaseColor();
                if (ImGui::ColorEdit3("Base Color", &baseColor.X))
                {
                    material->SetBaseColor(baseColor);
                }

                float metallic = material->GetMetallic();
                if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
                {
                    material->SetMetallic(metallic);
                }

                float roughness = material->GetRoughness();
                if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
                {
                    material->SetRoughness(roughness);
                }

                float shininess = material->GetShininess();
                if (ImGui::DragFloat("Shininess", &shininess, 1.0f, 1.0f, 256.0f))
                {
                    material->SetShininess(shininess);
                }

                // Base Color Texture
                auto texture = material->GetBaseColorTexture();
                if (texture)
                {
                    ImGui::Text("Base Color Texture:");
                    ImGui::Image((ImTextureID)texture->GetSrvHandle().ptr, ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("Base Color Texture: (None)");
                    ImGui::Dummy(ImVec2(64, 64));
                }

                // Base Color Texture 드롭 처리 (AssetHandle 함께 보관)
                if (auto handleOpt = HandleAssetDropTarget(resourceManager))
                {
                    if (resourceManager.GetAssetType(*handleOpt) == AssetType::Texture)
                    {
                        auto newTexture = resourceManager.GetOrLoadTexture(*handleOpt);
                        if (newTexture)
                        {
                            material->SetBaseColorTexture(newTexture, *handleOpt);
                        }
                    }
                }

                ImGui::Separator();

                // Normal Map
                auto normalTexture = material->GetNormalTexture();
                if (normalTexture)
                {
                    ImGui::Text("Normal Map:");
                    ImGui::Image((ImTextureID)normalTexture->GetSrvHandle().ptr, ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("Normal Map: (None)");
                    ImGui::Dummy(ImVec2(64, 64));
                }

                // Normal Map 드롭 처리 (AssetHandle 함께 보관)
                if (auto handleOpt = HandleAssetDropTarget(resourceManager))
                {
                    if (resourceManager.GetAssetType(*handleOpt) == AssetType::Texture)
                    {
                        auto newTexture = resourceManager.GetOrLoadTexture(*handleOpt);
                        if (newTexture)
                        {
                            material->SetNormalTexture(newTexture, *handleOpt);
                        }
                    }
                }
            }
        }

        ImGui::PopID();
    }

    // 3. Animator 영역
    if (AnimatorComponent* animator = selected->GetComponent<AnimatorComponent>())
    {
        ImGui::PushID("AnimatorComponent");

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
                            animator->CrossFade(clip->Name, 0.3f);
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

        ImGui::PopID();
    }

    // 인스펙터 패널 빈 곳 모델 에셋 드롭 처리 (ModelUtility 활용)
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (auto handleOpt = HandleAssetDropTarget(resourceManager))
    {
        if (resourceManager.GetAssetType(*handleOpt) == AssetType::Model)
        {
            auto model = resourceManager.GetOrLoadModel(*handleOpt);
            if (model)
            {
                ModelUtility::ReplaceModel(*selected, model, *handleOpt);
            }
        }
    }

    ImGui::PopID();
    ImGui::End();
}