#include "pch.h"
#include "Gizmo.h"
#include "EditorState.h"
#include "GameObject.h"
#include "Transform.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include "Quaternion.h"
#include "imgui.h"
#include "ImGuizmo.h"

void Gizmo::Draw(EditorState& editorState, const Camera& camera, float viewportX, float viewportY, float viewportWidth, float viewportHeight)
{
    // ① Viewport가 포커스된 상태이고 텍스트 입력 중이 아닐 때만 단축키 처리
    if (editorState.IsViewportFocused() && !ImGui::GetIO().WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) mOperation = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) mOperation = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) mOperation = 2;
        if (ImGui::IsKeyPressed(ImGuiKey_X)) mIsLocal = !mIsLocal; // X 키로 Local/World 토글
    }

    auto selected = editorState.GetSelected();
    if (!selected)
    {
        return;
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportX, viewportY, viewportWidth, viewportHeight);

    // 모드 선택 오버레이 UI (뷰포트 좌상단 배치)
    ImGui::SetNextWindowPos(ImVec2(viewportX + 8, viewportY + 8), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("GizmoControls", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

    if (ImGui::RadioButton("Move (W)", mOperation == 0)) mOperation = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate (E)", mOperation == 1)) mOperation = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale (R)", mOperation == 2)) mOperation = 2;

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // ② Local / World 토글 라디오 버튼
    if (ImGui::RadioButton("World", !mIsLocal)) mIsLocal = false;
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", mIsLocal)) mIsLocal = true;

    ImGui::End();

    ImGuizmo::OPERATION op = (mOperation == 0) ? ImGuizmo::TRANSLATE
        : (mOperation == 1) ? ImGuizmo::ROTATE
        : ImGuizmo::SCALE;

    ImGuizmo::MODE mode = mIsLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    // Scale 모드일 때는 강제로 LOCAL 처리 (World Scale 기즈모 왜곡 방지)
    if (op == ImGuizmo::SCALE)
    {
        mode = ImGuizmo::LOCAL;
    }

    Transform& transform = selected->GetTransform();

    Matrix4x4 world = transform.GetMatrix();
    Matrix4x4 view = camera.GetView();
    Matrix4x4 proj = camera.GetProjection();

    // row-major -> ImGuizmo 전용 column-major 전치
    Matrix4x4 worldT = world.GetTranspose();
    Matrix4x4 viewT = view.GetTranspose();
    Matrix4x4 projT = proj.GetTranspose();

    // ③ Ctrl 키 스냅 처리 (Translate: 1m, Rotate: 15도, Scale: 0.5)
    bool useSnap = ImGui::GetIO().KeyCtrl;
    float snapValue = (op == ImGuizmo::TRANSLATE) ? 1.0f
        : (op == ImGuizmo::ROTATE) ? 15.0f
        : 0.5f;

    float snapValues[3] = { snapValue, snapValue, snapValue };

    if (ImGuizmo::Manipulate(
        (const float*)viewT, (const float*)projT,
        op, mode,
        (float*)worldT,
        nullptr,
        useSnap ? snapValues : nullptr))
    {
        // 전치된 결과를 다시 원본 행렬 공간으로 전치
        Matrix4x4 newWorld = worldT.GetTranspose();

        // ⑦ Matrix -> Quaternion/Vector3 기반 정밀 분해 (Euler 오차 해소)
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;

        if (newWorld.Decompose(position, rotation, scale))
        {
            transform.SetPosition(position);
            transform.SetRotation(rotation);
            transform.SetScale(scale);
        }
    }
}