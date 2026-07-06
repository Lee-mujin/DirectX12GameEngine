#pragma once

class Input
{
public:
    void Update(); //매 프레임 맨 앞에서 호출

    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);
    void OnMouseMove(int x, int y);
    void OnMouseDown(int button);
    void OnMouseUp(int button);

    bool IsKeyDown(int key) const;
    bool IsKeyPressed(int key) const;
    bool IsKeyReleased(int key) const;

    bool IsMouseDown(int button) const;
    bool IsMousePressed(int button) const;

    int GetMouseX() const { return mMouseX; }
    int GetMouseY() const { return mMouseY; }
    int GetMouseDeltaX() const { return mMouseX - mPrevMouseX; }
    int GetMouseDeltaY() const { return mMouseY - mPrevMouseY; }

private:
    static constexpr int kKeyCount = 256;
    static constexpr int kMouseButtonCount = 3; //좌우중

    bool mCurrKeys[kKeyCount] = {};
    bool mPrevKeys[kKeyCount] = {};

    bool mCurrMouseButtons[kMouseButtonCount] = {};
    bool mPrevMouseButtons[kMouseButtonCount] = {};

    int mMouseX = 0;
    int mMouseY = 0;
    int mPrevMouseX = 0;
    int mPrevMouseY = 0;
};