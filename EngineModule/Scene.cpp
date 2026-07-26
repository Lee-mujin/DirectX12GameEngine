// Scene.cpp
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

void Scene::Render(D3D12Renderer& renderer)
{
    if (!mMainCamera) return;

    //메인 패스 시작 (카메라 및 조명 CB 업데이트)
    renderer.BeginFrame(mMainCamera->GetCamera(), mMainLight, mPointLight, mSpotLight);

    //씬 오브젝트들 메인 렌더링
    for (auto& obj : mObjects)
    {
        if (obj)
        {
            obj->Render(renderer);
        }
    }

    //메인 패스 마무리 (스왑체인 백버퍼 전환)
    renderer.TransitionToSwapChain();
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

void Scene::RenderShadowPass(D3D12Renderer& renderer)
{
    if (!mMainCamera) return;

    //메인 카메라 정보를 함께 넘겨서 CSM 계산이 가능하도록 인자 추가
    renderer.BeginShadowPass(mMainLight, mMainCamera->GetCamera());

    for (auto& obj : mObjects)
    {
        obj->RenderShadow(renderer);
    }

    renderer.EndShadowPass();
}