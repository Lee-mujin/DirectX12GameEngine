#pragma once
class EditorState;
class Camera;

class Gizmo
{
public:
    Gizmo() = default;
    ~Gizmo() = default;

    void Draw(EditorState& editorState, const Camera& camera, float viewportX, float viewportY, float viewportWidth, float viewportHeight);

private:
    int mOperation = 0;   // 0 = Translate, 1 = Rotate, 2 = Scale
    bool mIsLocal = false; // false = World, true = Local
};