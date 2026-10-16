#include "pch.h"
#include "Viewport.h"
#include "EditorState.h"
#include "Camera.h"
#include "Scene.h"
#include "Input.h"
#include "PickingSystem.h"
#include "RayCastHit.h"
#include "Vector2.h"
#include "imgui.h"
#include "ImGuizmo.h"

void Viewport::Draw(
    EditorState& editorState,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    float& outWidth, float& outHeight,
    float& outPosX, float& outPosY,
    Camera* camera,
    Scene* scene,
    Input* input)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    //현재 Viewport 패널이 활성화 상태인지 EditorState에 기록
    editorState.SetViewportFocused(ImGui::IsWindowFocused());

    ImVec2 size = ImGui::GetContentRegionAvail();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    outWidth = size.x;
    outHeight = size.y;
    outPosX = pos.x;
    outPosY = pos.y;

    //Viewport 이미지 출력
    ImGui::Image((ImTextureID)textureHandle.ptr, size);

    //Viewport 마우스 클릭 피킹(Picking) 연산 (기즈모 조작 중에는 피킹 방지)
    if (camera && scene && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
    {
        ImVec2 mousePos = ImGui::GetMousePos();

        //Viewport 텍스처 영역 내 상대 마우스 좌표 계산
        Vector2 relativeMousePos(
            mousePos.x - outPosX,
            mousePos.y - outPosY
        );

        //카메라를 통해 Viewport 상대 마우스 좌표를 World Ray로 변환
        Ray ray = camera->ScreenPointToRay(relativeMousePos, outWidth, outHeight);

        //PickingSystem으로 씬 내 오브젝트 검사
        RayCastHit hit;
        if (PickingSystem::Raycast(scene, ray, hit))
        {
            //클릭한 오브젝트를 EditorState에 전달
            editorState.SetSelected(hit.Object);
        }
        else
        {
            //허공 클릭 시 선택 해제
            editorState.SetSelected(nullptr);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}