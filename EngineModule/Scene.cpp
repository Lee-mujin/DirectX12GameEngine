#include "pch.h"
#include "Scene.h"
#include "D3D12Renderer.h"

void Scene::Update(Input* input, float deltaTime)
{
    if (mMainCamera)
    {
        mMainCamera->UpdateCamera(input, deltaTime);
    }

    for (auto& obj : mObjects)
    {
        obj->Update(deltaTime);
    }
}
//Scene 안의 카메라 GameObject가 알아서 업데이트 되도록 구현
void Scene::Render(D3D12Renderer& renderer)
{
    if (!mMainCamera) return;

    renderer.BeginFrame(mMainCamera->GetCamera(), mMainLight, mPointLight, mSpotLight);

    for (auto& obj : mObjects)
    {
        obj->Render(renderer);
    }

    renderer.EndFrame();
}

void Scene::AddGameObject(std::shared_ptr<GameObject> object)
{
    mObjects.push_back(object);
}

void Scene::ClearAssetObjects()
{
    mObjects.erase(
        std::remove_if(mObjects.begin(), mObjects.end(),
            [](const std::shared_ptr<GameObject>& obj)
            {
                return obj->GetSourceAssetHandle().IsValid();
            }),
        mObjects.end());
}