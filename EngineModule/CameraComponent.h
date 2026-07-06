#pragma once
#include "Component.h"
#include "Camera.h"

class Input;

class CameraComponent : public Component
{
public:
    void Initialize(float aspect);
    void UpdateCamera(Input* input, float deltaTime);

    Camera& GetCamera() { return mCamera; }
    const Camera& GetCamera() const { return mCamera; }

private:
    Camera mCamera;
};