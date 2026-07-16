#pragma once

class EditorState;
class Camera;

class Gizmo
{
public:
    void Draw(EditorState& editorState, const Camera& camera);

private:
    int mOperation = 0; // ImGuizmo::OPERATION 캐스팅용 (Translate/Rotate/Scale)
};