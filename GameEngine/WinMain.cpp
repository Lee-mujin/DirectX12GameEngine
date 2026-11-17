#include "pch.h"
#include <exception>
#include <string>
#include "Application.h"   // Engine 추가 포함 디렉터리 설정 필요

namespace
{
    void ReportFatal(const char* title, const std::string& detail)
    {
        OutputDebugStringA(detail.c_str());
        MessageBoxA(nullptr, detail.c_str(), title, MB_ICONERROR | MB_OK);
    }
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ int nCmdShow)
{
    Application app;
    bool initialized = false;

    try
    {
        if (!app.Initialize(hInstance))
        {
            ReportFatal("DX12 Game Engine - Initialize failed",
                "Application::Initialize() returned false. Check the debug output / debug layer.");
            return 1;
        }
        initialized = true;

        app.Run();
    }
    catch (const std::exception& e)
    {
        std::string detail = "Unhandled exception:\n";
        detail += e.what();
        ReportFatal("DX12 Game Engine - Fatal error", detail);
        return 1;
    }
    catch (...)
    {
        ReportFatal("DX12 Game Engine - Fatal error", "Unknown unhandled exception.");
        return 1;
    }

    if (initialized)
    {
        app.Shutdown();
    }
    return 0;
}
