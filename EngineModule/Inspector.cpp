#include "pch.h"
#include "Inspector.h"
#include "EditorState.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "imgui.h"
#include "Animation.h"
#include "ResourceManager.h" 

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

    // 각 위젯 타이틀이나 라벨이 겹쳐서 작동 불능 상태가 되지 않도록 최상단 격리
    ImGui::PushID(selected.get());

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Transform& transform = selected->GetTransform();

        Vector3 position = transform.GetPosition();
        if (ImGui::DragFloat3("Position", &position.X, 0.1f))
        {
            transform.SetPosition(position);
        }

        //오일러 각을 임구이로 제어할 때 데이터 정돈
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
        //머티리얼 컴포넌트 영역 ID 세션 격리
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

                auto texture = material->GetBaseColorTexture();
                if (texture)
                {
                    ImGui::Text("Base Color Texture:");
                    ImGui::Image((ImTextureID)texture->GetSrvHandle().ptr, ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("Base Color Texture: (None)");
                }

                if (ImGui::Button("Change Base Color Texture..."))
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
                        auto newTexture = resourceManager.LoadTexture(filePath);
                        if (newTexture)
                        {
                            material->SetBaseColorTexture(newTexture);
                        }
                    }
                }

                ImGui::Separator();

                auto normalTexture = material->GetNormalTexture();
                ImGui::Text("Normal Map: %s", normalTexture ? "Loaded" : "(None) - 셰이더 미적용, 데이터만 보관");
            }
            else
            {
                ImGui::Text("(No Material)");
            }
        }

        ImGui::PopID(); // MaterialComponent ID 해제
    }

    if (AnimatorComponent* animator = selected->GetComponent<AnimatorComponent>())
    {
        // 애니메이터 컴포넌트 영역 ID 세션 격리
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

        ImGui::PopID(); // AnimatorComponent ID 해제
    }

    // [콘텐트 브라우저 연동 강화] 
    // 인스펙터 패널 빈 곳 가득 더미 채우기 후 에셋 드롭 처리
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET"))
        {
            std::string pathStr(static_cast<const char*>(payload->Data));
            
            // 현재 선택된 오브젝트의 메쉬 소스를 던진 에셋의 메쉬 파일로 런타임 교체
            if (MeshRenderer* mr = selected->GetComponent<MeshRenderer>())
            {
                // 외부 모델 로더나 리소스 매니저 캐시를 사용해 메쉬 데이터 갱신 교체 처리 가능
                // auto newMesh = ResourceManager::GetInstance().GetOrLoadStaticModel(pathStr);
                // if(newMesh) mr->SetMesh(newMesh);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopID(); // 최상단 오브젝트 고유 ID 해제
    ImGui::End();
}