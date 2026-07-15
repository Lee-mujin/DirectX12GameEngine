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

        if (fileEntry.IsDirectory)
        {
            if (ImGui::TreeNode(fileEntry.Name.c_str()))
            {
                DrawDirectory(fileEntry.Path);
                ImGui::TreePop();
            }
        }
        else
        {
            bool isSelected = false;
            ImGui::Selectable(fileEntry.Name.c_str(), isSelected);

            // 중요: BeginDragDropSource가 "true"를 반환했을 때만 내부에서 EndDragDropSource()를 호출해야 합니다.
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                std::string pathStr = fileEntry.Path.string();

                // 데이터 전달 페이로드 설정
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET", pathStr.c_str(), pathStr.size() + 1);

                // 드래그 레이블 표시
                ImGui::Text("%s", fileEntry.Name.c_str());

                // true 블록 내부에서 닫아야 함
                ImGui::EndDragDropSource();
            }

            // 더블 클릭 이벤트 처리
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (mOnActivate)
                {
                    mOnActivate(fileEntry.Path);
                }
            }
        }
    }
}