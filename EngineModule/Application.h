#pragma once
#include <filesystem>
#include "Window.h"
#include "Timer.h"
#include "Input.h"
#include "D3D12Renderer.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "ImGuiLayer.h"
#include "EditorState.h"
#include "Hierarchy.h"
#include "Inspector.h"
#include "Light.h"
#include "ContentBrowser.h"

class Application
{
public:
    bool Initialize(HINSTANCE hInstance);
    void Run();
    void Shutdown();

private:
    void SpawnAssetIntoScene(const std::filesystem::path& assetPath);
private:
    Window mWindow;
    Timer mTimer;
    Input mInput;
    D3D12Renderer mRenderer;
    SceneManager mSceneManager;
    ResourceManager mResourceManager;
    ImGuiLayer mImGuiLayer;
    EditorState mEditorState;
    Hierarchy mHierarchy;
    Inspector mInspector;
	Light mLight;
    ContentBrowser mContentBrowser;
};