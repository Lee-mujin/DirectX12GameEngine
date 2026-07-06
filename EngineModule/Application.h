#pragma once
#include "Window.h"
#include "Timer.h"
#include "Input.h"
#include "D3D12Renderer.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "ImGuiLayer.h"

class Application
{
public:
    bool Initialize(HINSTANCE hInstance);
    void Run();
    void Shutdown();

private:
    Window mWindow;
    Timer mTimer;
    Input mInput;
    D3D12Renderer mRenderer;
    SceneManager mSceneManager;
    ResourceManager mResourceManager;
    ImGuiLayer mImGuiLayer;
};