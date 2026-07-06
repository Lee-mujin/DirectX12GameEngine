#pragma once
#include <vector>
#include <memory>
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

class GameObject;
class D3D12Renderer;
class CameraComponent;
class Input;

class Scene
{
public:
    void Update(Input* input, float deltaTime);
    void Render(D3D12Renderer& renderer);

    void AddGameObject(std::shared_ptr<GameObject> object);
    void SetMainCamera(CameraComponent* camera) { mMainCamera = camera; }
    void SetMainLight(const DirectionalLight& light) { mMainLight = light; }
    void SetPointLight(const PointLight& light) { mPointLight = light; }
    void SetSpotLight(const SpotLight& light) { mSpotLight = light; }

private:
    std::vector<std::shared_ptr<GameObject>> mObjects;
    CameraComponent* mMainCamera = nullptr;
    DirectionalLight mMainLight;
    PointLight mPointLight;
    SpotLight mSpotLight;
};