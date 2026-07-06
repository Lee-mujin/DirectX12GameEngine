#pragma once

class Input;

class Window
{
public:
    bool Create(HINSTANCE hInstance, int width, int height);
    bool ProcessMessage();
    void Destroy();

    void SetInput(Input* input) { mInput = input; }

    HWND GetHWND() const { return mHwnd; }
    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

private:
    HWND mHwnd = nullptr;
    HINSTANCE mInstance = nullptr;
    int mWidth = 0;
    int mHeight = 0;

    Input* mInput = nullptr; // Window가 소유하지 않음, Application이 연결만 해줌

    const TCHAR* mClassName = _T("DX12_APP");
    TCHAR mTitle[64] = _T("DX12 App");
};