#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "CameraComponent.h"

void Scene::Update(Input* input, float deltaTime)
{
    if (mMainCamera)
    {
        mMainCamera->UpdateCamera(input, deltaTime);
    }

    //애니메이션 시스템 일괄 업데이트
    mAnimationSystem.Update(mObjects, deltaTime);

    //일반 게임오브젝트 로직 업데이트
    for (auto& obj : mObjects)
    {
        if (obj)
        {
            obj->Update(deltaTime);
        }
    }
}

void Scene::BuildRenderQueue(RenderQueue& outQueue)
{
    //렌더 시스템을 통해 RenderItem 패킷 일괄 수집
    mRenderSystem.BuildRenderQueue(mObjects, outQueue);
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