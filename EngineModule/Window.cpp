#include "pch.h"
#include "Window.h"
#include "Input.h"

bool Window::Create(HINSTANCE hInstance, int width, int height)
{
    mInstance = hInstance;
    mWidth = width;
    mHeight = height;

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = mInstance;
    wc.hIcon = ::LoadIcon(0, IDI_APPLICATION);
    wc.hIconSm = ::LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = ::LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = mClassName;

    if (!::RegisterClassEx(&wc))
    {
        return false;
    }

    RECT rect{ 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    mHwnd = ::CreateWindow(
        mClassName, mTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        NULL, NULL, mInstance, NULL);

    if (!mHwnd)
    {
        return false;
    }

    ::SetWindowLongPtr(mHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    ::ShowWindow(mHwnd, SW_SHOW);
    ::UpdateWindow(mHwnd);

    return true;
}

bool Window::ProcessMessage()
{
    MSG msg;
    ::ZeroMemory(&msg, sizeof(msg));

    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        ::DispatchMessage(&msg);

        if (msg.message == WM_QUIT)
        {
            return false;
        }
    }

    return true;
}

void Window::Destroy()
{
    if (mHwnd)
    {
        ::DestroyWindow(mHwnd);
        mHwnd = nullptr;
    }

    ::UnregisterClass(mClassName, mInstance);
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
{
    Window* window = reinterpret_cast<Window*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        if (window && window->mInput) window->mInput->OnKeyDown(wParam);
        break;

    case WM_KEYUP:
        if (window && window->mInput) window->mInput->OnKeyUp(wParam);
        break;

    case WM_MOUSEMOVE:
        if (window && window->mInput) window->mInput->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_LBUTTONDOWN:
        if (window && window->mInput) window->mInput->OnMouseDown(0);
        break;
    case WM_LBUTTONUP:
        if (window && window->mInput) window->mInput->OnMouseUp(0);
        break;

    case WM_RBUTTONDOWN:
        if (window && window->mInput) window->mInput->OnMouseDown(1);
        break;
    case WM_RBUTTONUP:
        if (window && window->mInput) window->mInput->OnMouseUp(1);
        break;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}