#include "pch.h"
#include "Application.h"   // Engine 추가 포함 디렉터리 설정 필요

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ int nCmdShow)
{
    Application app;
    if (!app.Initialize(hInstance))
    {
        return 0;
    }

    app.Run();
    app.Shutdown();

    return 0;
}