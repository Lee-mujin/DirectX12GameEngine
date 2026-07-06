#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"

bool SceneManager::Initialize()
{
    return true;
}

void SceneManager::Update(Input* input, float deltaTime)
{
    if (mCurrentScene)
    {
        mCurrentScene->Update(input, deltaTime);
    }
}

void SceneManager::Render(D3D12Renderer& renderer)
{
    if (mCurrentScene)
    {
        mCurrentScene->Render(renderer);
    }
}

void SceneManager::ChangeScene(std::shared_ptr<Scene> scene)
{
    mCurrentScene = scene;
}