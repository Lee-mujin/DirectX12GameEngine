#include "pch.h"
#include "ContentBrowser.h"
#include "imgui.h"

namespace fs = std::filesystem;

void ContentBrowser::Initialize(const std::string& rootPath, ActivateCallback onActivate)
{
    mRootPath = rootPath;
    mOnActivate = onActivate;
}

void ContentBrowser::Draw()
{
    ImGui::Begin("Content Browser");

    if (fs::exists(mRootPath))
    {
        DrawDirectory(mRootPath);
    }
    else
    {
        ImGui::Text("Assets 폴더를 찾을 수 없습니다: %s", mRootPath.string().c_str());
    }

    ImGui::End();
}

void ContentBrowser::DrawDirectory(const fs::path& dirPath)
{
    for (const auto& entry : fs::directory_iterator(dirPath))
    {
        FileEntry fileEntry;
        fileEntry.Path = entry.path();
        fileEntry.Name = entry.path().filename().string();
        fileEntry.Extension = entry.path().extension().string();
        fileEntry.IsDirectory = entry.is_directory();

        // 1. 디렉터리(폴더) 노드 처리
        if (fileEntry.IsDirectory)
        {
            // 폴더 단위도 혹시 모를 ID 충돌 방지를 위해 PushID 처리
            ImGui::PushID(fileEntry.Name.c_str());
            if (ImGui::TreeNode(fileEntry.Name.c_str()))
            {
                DrawDirectory(fileEntry.Path);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        // 2. 파일 항목 처리
        else
        {
            // [중요] 각 파일 에셋마다 고유 ID 영역을 격리하여 다른 위젯/스택과의 간섭을 원천 차단합니다.
            std::string uniqueID = "FILE_" + fileEntry.Path.string();
            ImGui::PushID(uniqueID.c_str());

            bool isSelected = false;
            ImGui::Selectable(fileEntry.Name.c_str(), isSelected);

            // 드래그 소스 설정 (안전한 명시적 플래그 적용)
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoDisableHover))
            {
                std::string pathStr = fileEntry.Path.string();

                // 데이터 전달 페이로드 설정
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET", pathStr.c_str(), pathStr.size() + 1);

                // 드래그하는 동안 마우스 옆에 따라다닐 텍스트 프리뷰
                ImGui::Text("%s", fileEntry.Name.c_str());

                // 조건문 범위 내부에서만 정확히 안전하게 해제
                ImGui::EndDragDropSource();
            }

            // 마우스 더블 클릭 처리
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (mOnActivate)
                {
                    mOnActivate(fileEntry.Path);
                }
            }

            // 격리했던 파일의 ID 영역 해제
            ImGui::PopID();
        }
    }
}