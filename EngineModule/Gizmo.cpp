#include "pch.h"
#include "Gizmo.h"
#include "EditorState.h"
#include "GameObject.h"
#include "Transform.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include "imgui.h"
#include "ImGuizmo.h"

void Gizmo::Draw(EditorState& editorState, const Camera& camera)
{
    auto selected = editorState.GetSelected();
    if (!selected)
    {
        return;
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    // 조작 모드 선택 UI (작은 오버레이 창)
    ImGui::Begin("Gizmo");
    if (ImGui::RadioButton("Translate", mOperation == 0)) mOperation = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mOperation == 1)) mOperation = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mOperation == 2)) mOperation = 2;
    ImGui::End();

    ImGuizmo::OPERATION op = mOperation == 0 ? ImGuizmo::TRANSLATE
        : mOperation == 1 ? ImGuizmo::ROTATE
        : ImGuizmo::SCALE;

    Transform& transform = selected->GetTransform();

    Matrix4x4 world = transform.GetMatrix();
    Matrix4x4 view = camera.GetView();
    Matrix4x4 proj = camera.GetProjection();

    // Matrix4x4는 row-major, ImGuizmo는 column-major 배열(float[16])을 기대하므로 전치해서 넘김
    Matrix4x4 worldT = world.GetTranspose();
    Matrix4x4 viewT = view.GetTranspose();
    Matrix4x4 projT = proj.GetTranspose();

    if (ImGuizmo::Manipulate(
        (const float*)viewT, (const float*)projT,
        op, ImGuizmo::WORLD,
        (float*)worldT))
    {
        // 조작된 결과를 다시 row-major로 되돌리고 TRS로 분해
        Matrix4x4 newWorld = worldT.GetTranspose();

        float t[3], r[3], s[3];
        ImGuizmo::DecomposeMatrixToComponents((const float*)worldT, t, r, s);

        transform.SetPosition(Vector3(t[0], t[1], t[2]));
        transform.SetRotation(Vector3(r[0], r[1], r[2])); // degree
        transform.SetScale(Vector3(s[0], s[1], s[2]));
    }
}