#pragma once
#include <vector>
#include <memory>
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "RenderQueue.h"
#include "AnimationSystem.h"
#include "RenderSystem.h"

class GameObject;
class CameraComponent;
class Input;

class Scene
{
public:
    void Update(Input* input, float deltaTime);

    void BuildRenderQueue(RenderQueue& outQueue);

    void AddGameObject(std::shared_ptr<GameObject> object);
    void SetMainCamera(CameraComponent* camera) { mMainCamera = camera; }
    void SetMainLight(const DirectionalLight& light) { mMainLight = light; }
    void SetPointLight(const PointLight& light) { mPointLight = light; }
    void SetSpotLight(const SpotLight& light) { mSpotLight = light; }
    void ClearAssetObjects();

public:
    DirectionalLight& GetMainLight() { return mMainLight; }
    PointLight& GetPointLight() { return mPointLight; }
    SpotLight& GetSpotLight() { return mSpotLight; }
    CameraComponent* GetMainCamera() const { return mMainCamera; }
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const { return mObjects; }

private:
    std::vector<std::shared_ptr<GameObject>> mObjects;
    CameraComponent* mMainCamera = nullptr;
    DirectionalLight mMainLight;
    PointLight mPointLight;
    SpotLight mSpotLight;

    //전용 시스템 멤버
    AnimationSystem mAnimationSystem;
    RenderSystem mRenderSystem;
};